// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/except.h"
#include "scipp/core/groupby.h"
#include "scipp/core/histogram.h"
#include "scipp/core/indexed_slice_view.h"
#include "scipp/core/tag_util.h"

#include "dataset_operations_common.h"
#include "variable_operations_common.h"

namespace scipp::core {

/// Helper for creating output for "combine" step for "apply" steps that reduce
/// a dimension.
///
/// - Delete anything (but data) that depends on the reduction dimension.
/// - Default-init data.
template <class T>
T GroupBy<T>::makeReductionOutput(const Dim reductionDim) const {
  auto out = resize(m_data, reductionDim, size());
  out.rename(reductionDim, dim());
  out.setCoord(dim(), key());
  return out;
}

/// Flatten provided dimension in each group and return combined data.
///
/// This only supports sparse data.
template <class T> T GroupBy<T>::flatten(const Dim reductionDim) const {
  auto out = makeReductionOutput(reductionDim);
  const auto apply = [&](const auto &out_, const auto &in,
                         const scipp::index group_) {
    const Dim sparseDim = in.dims().sparseDim();
    for (const auto &slice : groups()[group_]) {
      const auto &array = in.slice(slice);
      flatten_impl(out_.coords()[sparseDim], array.coords()[sparseDim]);
      if (in.hasData())
        flatten_impl(out_.data(), array.data());
      for (auto &&[label_name, label] : out_.labels()) {
        if (label.dims().sparse())
          flatten_impl(label, array.labels()[label_name]);
      }
    }
  };
  // Apply to each group, storing result in output slice
  for (scipp::index group = 0; group < size(); ++group) {
    const auto out_slice = out.slice({dim(), group});
    if constexpr (std::is_same_v<T, Dataset>) {
      for (const auto &[name, item] : m_data)
        apply(out_slice[name], item, group);
    } else {
      apply(out_slice, m_data, group);
    }
  }
  return out;
}

/// Apply sum to groups and return combined data.
template <class T> T GroupBy<T>::sum(const Dim reductionDim) const {
  auto out = makeReductionOutput(reductionDim);
  // Apply to each group, storing result in output slice
  for (scipp::index group = 0; group < size(); ++group) {
    const auto out_slice = out.slice({dim(), group});
    if constexpr (std::is_same_v<T, Dataset>) {
      for (const auto &[name, item] : m_data) {
        const auto out_data = out_slice[name].data();
        for (const auto &slice : groups()[group]) {
          sum_impl(out_data, item.data().slice(slice));
        }
      }
    } else {
      const auto out_data = out_slice.data();
      for (const auto &slice : groups()[group]) {
        sum_impl(out_data, m_data.data().slice(slice));
      }
    }
  }
  return out;
}

/// Apply mean to groups and return combined data.
template <class T> T GroupBy<T>::mean(const Dim reductionDim) const {
  // 1. Sum into output slices
  auto out = sum(reductionDim);

  // 2. Compute number of slices N contributing to each out slice
  auto scale = makeVariable<double>({dim(), size()});
  const auto scaleT = scale.template values<double>();
  for (scipp::index group = 0; group < size(); ++group)
    for (const auto &slice : groups()[group])
      scaleT[group] += slice.end() - slice.begin();
  scale = 1.0 / scale;

  // 3. sum/N -> mean
  if constexpr (std::is_same_v<T, Dataset>) {
    for (const auto &[name, item] : out) {
      if (isInt(item.data().dtype()))
        out.setData(name, item.data() * scale);
      else
        item *= scale;
    }
  } else {
    if (isInt(out.data().dtype()))
      out.setData(out.data() * scale);
    else
      out *= scale;
  }

  return out;
}

static void expectValidGroupbyKey(const VariableConstProxy &key) {
  if (key.dims().ndim() != 1)
    throw except::DimensionError("Group-by key must be 1-dimensional");
  if (key.hasVariances())
    throw except::VariancesError("Group-by key cannot have variances");
}

template <class T> struct MakeGroups {
  static auto apply(const VariableConstProxy &key, const Dim targetDim) {
    expectValidGroupbyKey(key);
    const auto &values = key.values<T>();

    const auto dim = key.dims().inner();
    std::map<T, std::vector<Slice>> indices;
    for (scipp::index i = 0; i < scipp::size(values);) {
      // Use contiguous (thick) slices if possible to avoid overhead of slice
      // handling in follow-up "apply" steps.
      const auto begin = i;
      const auto value = values[i];
      while (values[i] == value && i < scipp::size(values))
        ++i;
      indices[value].emplace_back(dim, begin, i);
    }

    const Dimensions dims{targetDim, scipp::size(indices)};
    std::vector<T> keys;
    std::vector<std::vector<Slice>> groups;
    for (auto &item : indices) {
      keys.push_back(item.first);
      groups.emplace_back(std::move(item.second));
    }
    auto keys_ = makeVariable<T>(dims, std::move(keys));
    keys_.setUnit(key.unit());
    return GroupByGrouping{std::move(keys_), std::move(groups)};
  }
};

template <class T> struct MakeBinGroups {
  static auto apply(const VariableConstProxy &key,
                    const VariableConstProxy &bins) {
    expectValidGroupbyKey(key);
    if (bins.dims().ndim() != 1)
      throw except::DimensionError("Group-by bins must be 1-dimensional");
    if (key.unit() != bins.unit())
      throw except::UnitError("Group-by key must have same unit as bins");
    const auto &values = key.values<T>();
    const auto &edges = bins.values<T>();
    expect::histogram::sorted_edges(edges);

    const auto dim = key.dims().inner();
    std::vector<std::vector<Slice>> groups(edges.size() - 1);
    for (scipp::index i = 0; i < scipp::size(values);) {
      // Use contiguous (thick) slices if possible to avoid overhead of slice
      // handling in follow-up "apply" steps.
      const auto value = values[i];
      const auto begin = i++;
      auto right = std::upper_bound(edges.begin(), edges.end(), value);
      if (right != edges.end() && right != edges.begin()) {
        auto left = right - 1;
        while ((*left <= values[i]) && (values[i] < *right) &&
               i < scipp::size(values))
          ++i;
        groups[std::distance(edges.begin(), left)].emplace_back(dim, begin, i);
      }
    }
    return GroupByGrouping{Variable(bins), std::move(groups)};
  }
};

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by `labels`.
/// Grouping of labels will create a new coordinate for `targetDim` in a later
/// apply/combine step.
GroupBy<DataArray> groupby(const DataConstProxy &array,
                           const std::string &labels, const Dim targetDim) {
  const auto &key = array.labels()[labels];
  return {array, CallDType<double, float, int64_t, int32_t, bool,
                           std::string>::apply<MakeGroups>(key.dtype(), key,
                                                           targetDim)};
}

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by `labels`.
/// Grouping of labels is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<DataArray> groupby(const DataConstProxy &array,
                           const std::string &labels,
                           const VariableConstProxy &bins) {
  const auto &key = array.labels()[labels];
  return {array,
          CallDType<double, float, int64_t, int32_t>::apply<MakeBinGroups>(
              key.dtype(), key, bins)};
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by `labels`.
/// Grouping of labels will create a new coordinate for `targetDim` in a later
/// apply/combine step.
GroupBy<Dataset> groupby(const DatasetConstProxy &dataset,
                         const std::string &labels, const Dim targetDim) {
  const auto &key = dataset.labels()[labels];
  return {dataset, CallDType<double, float, int64_t, int32_t, bool,
                             std::string>::apply<MakeGroups>(key.dtype(), key,
                                                             targetDim)};
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by `labels`.
/// Grouping of labels is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<Dataset> groupby(const DatasetConstProxy &dataset,
                         const std::string &labels,
                         const VariableConstProxy &bins) {
  const auto &key = dataset.labels()[labels];
  return {dataset,
          CallDType<double, float, int64_t, int32_t>::apply<MakeBinGroups>(
              key.dtype(), key, bins)};
}

template class GroupBy<DataArray>;
template class GroupBy<Dataset>;

} // namespace scipp::core
