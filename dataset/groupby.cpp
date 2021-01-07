// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/bucket.h"
#include "scipp/core/histogram.h"
#include "scipp/core/parallel.h"
#include "scipp/core/tag_util.h"

#include "scipp/variable/indexed_slice_view.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/util.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/choose.h"
#include "scipp/dataset/dataset_util.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/reduction.h"
#include "scipp/dataset/shape.h"

#include "../variable/operations_common.h"
#include "bin_common.h"
#include "dataset_operations_common.h"

using namespace scipp::variable;

namespace scipp::dataset {

/// Extract given group as a new data array or dataset
template <class T>
T GroupBy<T>::copy(const scipp::index group,
                   const AttrPolicy attrPolicy) const {
  const auto &slices = groups()[group];
  scipp::index size = 0;
  for (const auto &slice : slices)
    size += slice.end() - slice.begin();
  // This is just the slicing dim, but `slices` may be empty
  const Dim slice_dim = m_data.coords()[dim()].dims().inner();
  auto out =
      scipp::dataset::copy(m_data.slice({slice_dim, 0, size}), attrPolicy);
  scipp::index current = 0;
  for (const auto &slice : slices) {
    const auto thickness = slice.end() - slice.begin();
    const Slice out_slice(slice_dim, current, current + thickness);
    scipp::dataset::copy(m_data.slice(slice), out.slice(out_slice), attrPolicy);
    current += thickness;
  }
  return out;
}

/// Helper for creating output for "combine" step for "apply" steps that reduce
/// a dimension.
///
/// - Delete anything (but data) that depends on the reduction dimension.
/// - Default-init data.
template <class T>
T GroupBy<T>::makeReductionOutput(const Dim reductionDim) const {
  T out;
  if (is_bins(m_data)) {
    const auto out_sizes =
        GroupBy(bucket_sizes(m_data), {key(), groups()}).sum(reductionDim);
    out = resize(m_data, reductionDim, out_sizes);
  } else {
    out = resize(m_data, reductionDim, size());
    out.rename(reductionDim, dim());
  }
  out.coords().set(dim(), key());
  return out;
}

template <class T>
template <class Op>
T GroupBy<T>::reduce(Op op, const Dim reductionDim) const {
  auto out = makeReductionOutput(reductionDim);
  const auto get_mask = [&](const auto &data) {
    auto mask = irreducible_mask(data.masks(), reductionDim);
    if (mask)
      mask = ~std::move(
          mask); // `op` multiplies mask into data to zero masked elements
    return mask;
  };
  // Apply to each group, storing result in output slice
  const auto process_groups = [&](const auto &range) {
    for (scipp::index group = range.begin(); group != range.end(); ++group) {
      const auto out_slice = out.slice({dim(), group});
      if constexpr (std::is_same_v<T, Dataset>) {
        for (const auto &item : m_data)
          op(out_slice[item.name()], item, groups()[group], reductionDim,
             get_mask(m_data[item.name()]));
      } else {
        op(out_slice, m_data, groups()[group], reductionDim, get_mask(m_data));
      }
    }
  };
  core::parallel::parallel_for(core::parallel::blocked_range(0, size()),
                               process_groups);
  return out;
}

namespace groupby_detail {

static constexpr auto sum =
    [](const DataArrayView &out, const auto &data_container,
       const GroupByGrouping::group &group, const Dim, const Variable &mask) {
      for (const auto &slice : group) {
        const auto data_slice = data_container.slice(slice);
        if (mask)
          sum_impl(out.data(), data_slice.data() * mask.slice(slice));
        else
          sum_impl(out.data(), data_slice.data());
      }
    };

template <void (*Func)(const VariableView &, const VariableConstView &)>
// The msvc compiler was failing to pass the templated function correctly
// requiring the introduction of a wrapping struct to aid compiler
// resolution.
struct wrap {
  static constexpr auto reduce_idempotent =
      [](const DataArrayView &out, const auto &data_container,
         const GroupByGrouping::group &group, const Dim reductionDim,
         const Variable &mask) {
        bool first = true;
        for (const auto &slice : group) {
          const auto data_slice = data_container.slice(slice);
          if (mask)
            throw std::runtime_error(
                "This operation does not support masks yet.");
          if (first) {
            out.data().assign(data_slice.data().slice({reductionDim, 0}));
            first = false;
          }
          Func(out.data(), data_slice.data());
        }
      };
};
} // namespace groupby_detail

/// Reduce each group by concatenating elements and return combined data.
///
/// This only supports binned data.
template <class T> T GroupBy<T>::concatenate(const Dim reductionDim) const {
  const auto concat = [&](const auto &data) {
    if (key().dims().volume() == scipp::size(groups()))
      return groupby_concat_bins(data, {}, key(), reductionDim);
    else
      return groupby_concat_bins(data, key(), {}, reductionDim);
  };
  if constexpr (std::is_same_v<T, DataArray>) {
    return concat(m_data);
  } else {
    return apply_to_items(m_data, [&](auto &&... _) { return concat(_...); });
  }
}

/// Reduce each group using `sum` and return combined data.
template <class T> T GroupBy<T>::sum(const Dim reductionDim) const {
  return reduce(groupby_detail::sum, reductionDim);
}

/// Reduce each group using `all` and return combined data.
template <class T> T GroupBy<T>::all(const Dim reductionDim) const {
  return reduce(groupby_detail::wrap<all_impl>::reduce_idempotent,
                reductionDim);
}

/// Reduce each group using `any` and return combined data.
template <class T> T GroupBy<T>::any(const Dim reductionDim) const {
  return reduce(groupby_detail::wrap<any_impl>::reduce_idempotent,
                reductionDim);
}

/// Reduce each group using `max` and return combined data.
template <class T> T GroupBy<T>::max(const Dim reductionDim) const {
  return reduce(groupby_detail::wrap<max_impl>::reduce_idempotent,
                reductionDim);
}

/// Reduce each group using `min` and return combined data.
template <class T> T GroupBy<T>::min(const Dim reductionDim) const {
  return reduce(groupby_detail::wrap<min_impl>::reduce_idempotent,
                reductionDim);
}

/// Apply mean to groups and return combined data.
template <class T> T GroupBy<T>::mean(const Dim reductionDim) const {
  // 1. Sum into output slices
  auto out = sum(reductionDim);

  // 2. Compute number of slices N contributing to each out slice
  const auto get_scale = [&](const auto &data) {
    auto scale = makeVariable<double>(Dims{dim()}, Shape{size()});
    const auto scaleT = scale.template values<double>();
    const auto mask = irreducible_mask(data.masks(), reductionDim);
    for (scipp::index group = 0; group < size(); ++group)
      for (const auto &slice : groups()[group]) {
        // N contributing to each slice
        scaleT[group] += slice.end() - slice.begin();
        // N masks for each slice, that need to be subtracted
        if (mask) {
          const auto masks_sum = variable::sum(mask.slice(slice), reductionDim);
          scaleT[group] -= masks_sum.template value<int64_t>();
        }
      }
    return reciprocal(std::move(scale));
  };

  // 3. sum/N -> mean
  if constexpr (std::is_same_v<T, Dataset>) {
    for (const auto &item : out) {
      if (isInt(item.data().dtype()))
        out.setData(item.name(), item.data() * get_scale(m_data[item.name()]),
                    AttrPolicy::Keep);
      else
        item *= get_scale(m_data[item.name()]);
    }
  } else {
    if (isInt(out.data().dtype()))
      out.setData(out.data() * get_scale(m_data));
    else
      out *= get_scale(m_data);
  }

  return out;
}

template <class T> struct MakeGroups {
  static auto apply(const VariableConstView &key, const Dim targetDim) {
    expect::isKey(key);
    const auto &values = key.values<T>();

    const auto dim = key.dims().inner();
    std::map<T, GroupByGrouping::group> indices;
    const auto end = values.end();
    scipp::index i = 0;
    for (auto it = values.begin(); it != end;) {
      // Use contiguous (thick) slices if possible to avoid overhead of slice
      // handling in follow-up "apply" steps.
      const auto begin = i;
      const auto &value = *it;
      while (it != end && *it == value) {
        ++it;
        ++i;
      }
      indices[value].emplace_back(dim, begin, i);
    }

    const Dimensions dims{targetDim, scipp::size(indices)};
    std::vector<T> keys;
    std::vector<GroupByGrouping::group> groups;
    for (auto &item : indices) {
      keys.emplace_back(std::move(item.first));
      groups.emplace_back(std::move(item.second));
    }
    auto keys_ = makeVariable<T>(Dimensions{dims}, Values(std::move(keys)));
    keys_.setUnit(key.unit());
    return GroupByGrouping{std::move(keys_), std::move(groups)};
  }
};

template <class T> struct MakeBinGroups {
  static auto apply(const VariableConstView &key,
                    const VariableConstView &bins) {
    expect::isKey(key);
    if (bins.dims().ndim() != 1)
      throw except::DimensionError("Group-by bins must be 1-dimensional");
    if (key.unit() != bins.unit())
      throw except::UnitError("Group-by key must have same unit as bins");
    const auto &values = key.values<T>();
    const auto &edges = bins.values<T>();
    core::expect::histogram::sorted_edges(edges);

    const auto dim = key.dims().inner();
    std::vector<GroupByGrouping::group> groups(edges.size() - 1);
    for (scipp::index i = 0; i < scipp::size(values);) {
      // Use contiguous (thick) slices if possible to avoid overhead of slice
      // handling in follow-up "apply" steps.
      const auto value = values[i];
      const auto begin = i++;
      auto right = std::upper_bound(edges.begin(), edges.end(), value);
      if (right != edges.end() && right != edges.begin()) {
        auto left = right - 1;
        while (i < scipp::size(values) && (*left <= values[i]) &&
               (values[i] < *right))
          ++i;
        groups[std::distance(edges.begin(), left)].emplace_back(dim, begin, i);
      }
    }
    return GroupByGrouping{Variable(bins), std::move(groups)};
  }
};

template <class T>
GroupBy<typename T::value_type> call_groupby(const T &array,
                                             const VariableConstView &key,
                                             const VariableConstView &bins) {
  return {
      array,
      core::CallDType<double, float, int64_t, int32_t>::apply<MakeBinGroups>(
          key.dtype(), key, bins)};
}

template <class T>
GroupBy<typename T::value_type>
call_groupby(const T &array, const VariableConstView &key, const Dim &dim) {
  return {array, core::CallDType<double, float, int64_t, int32_t, bool,
                                 std::string>::apply<MakeGroups>(key.dtype(),
                                                                 key, dim)};
}

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by a coord.
/// Grouping will create a new coordinate for the dimension of the grouping
/// coord in a later apply/combine step.
GroupBy<DataArray> groupby(const DataArrayConstView &array, const Dim dim) {
  const auto &key = array.coords()[dim];
  return call_groupby(array, key, dim);
}

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<DataArray> groupby(const DataArrayConstView &array, const Dim dim,
                           const VariableConstView &bins) {
  const auto &key = array.coords()[dim];
  return groupby(array, key, bins);
}

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<DataArray> groupby(const DataArrayConstView &array,
                           const VariableConstView &key,
                           const VariableConstView &bins) {
  if (!array.dims().contains(key.dims()))
    throw except::DimensionError("Size of Group-by key is incorrect.");

  return call_groupby(array, key, bins);
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by a coord.
/// Grouping will create a new coordinate for the dimension of the grouping
/// coord in a later apply/combine step.
GroupBy<Dataset> groupby(const DatasetConstView &dataset, const Dim dim) {
  const auto &key = dataset.coords()[dim];
  return call_groupby(dataset, key, dim);
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<Dataset> groupby(const DatasetConstView &dataset, const Dim dim,
                         const VariableConstView &bins) {
  const auto &key = dataset.coords()[dim];
  return groupby(dataset, key, bins);
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<Dataset> groupby(const DatasetConstView &dataset,
                         const VariableConstView &key,
                         const VariableConstView &bins) {
  for (const auto &n : dataset.dimensions()) {
    Dimensions dims(n.first, n.second);
    if (dims.contains(key.dims()))
      // Found compatible Dimension.
      return call_groupby(dataset, key, bins);
  }
  // No Dimension contains the key - throw.
  throw except::DimensionError("Size of Group-by key is incorrect.");
}

template class GroupBy<DataArray>;
template class GroupBy<Dataset>;

constexpr auto slice_by_value = [](const auto &x, const Dim dim,
                                   const auto &key) {
  const auto size = x.dims()[dim];
  const auto &coord = x.coords()[dim];
  for (scipp::index i = 0; i < size; ++i)
    if (coord.slice({dim, i}) == key)
      return x.slice({dim, i});
  throw std::runtime_error("Given key not found in coord.");
};

/// Similar to numpy.choose, but choose based on *values* in `key`.
///
/// Chooses slices of `choices` along `dim`, based on values of dimension-coord
/// for `dim`.
DataArray choose(const VariableConstView &key,
                 const DataArrayConstView &choices, const Dim dim) {
  const auto grouping = call_groupby(key, key, dim);
  const Dim target_dim = key.dims().inner();
  auto out = resize(choices, dim, key.dims()[target_dim]);
  out.rename(dim, target_dim);
  out.coords().set(dim, key); // not target_dim
  for (scipp::index group = 0; group < grouping.size(); ++group) {
    const auto value = grouping.key().slice({dim, group});
    const auto &choice = slice_by_value(choices, dim, value);
    for (const auto &slice : grouping.groups()[group]) {
      const auto out_ = out.slice(slice);
      out_.data().assign(broadcast(choice.data(), out_.dims()));
    }
  }
  return out;
}

} // namespace scipp::dataset
