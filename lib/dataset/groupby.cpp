// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/bucket.h"
#include "scipp/core/element/comparison.h"
#include "scipp/core/element/logical.h"
#include "scipp/core/histogram.h"
#include "scipp/core/parallel.h"
#include "scipp/core/tag_util.h"

#include "scipp/variable/accumulate.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/shape.h"
#include "scipp/dataset/util.h"

#include "../variable/operations_common.h"
#include "bin_common.h"
#include "dataset_operations_common.h"

using namespace scipp::variable;

namespace scipp::dataset {

namespace {
auto resize_array(const DataArray &da, const Dim reductionDim,
                  const scipp::index size, const FillValue fill) {
  if (!is_bins(da))
    return resize(da, reductionDim, size, fill);
  if (variableFactory().has_masks(da.data()))
    throw except::NotImplementedError(
        "Reduction operations for binned data with "
        "event masks not supported yet.");
  DataArray dense_dummy(da);
  dense_dummy.setData(empty(da.dims(), variableFactory().elem_unit(da.data()),
                            variableFactory().elem_dtype(da.data()),
                            variableFactory().has_variances(da.data())));
  return resize_array(dense_dummy, reductionDim, size, fill);
}
} // namespace

/// Helper for creating output for "combine" step for "apply" steps that reduce
/// a dimension.
///
/// - Delete anything (but data) that depends on the reduction dimension.
/// - Default-init data.
template <class T>
T GroupBy<T>::makeReductionOutput(const Dim reductionDim,
                                  const FillValue fill) const {
  T out;
  if constexpr (std::is_same_v<T, Dataset>) {
    out = apply_to_items(m_data, resize_array, reductionDim, size(), fill);
  } else {
    out = resize_array(m_data, reductionDim, size(), fill);
  }
  out = out.rename_dims({{reductionDim, dim()}});
  out.coords().set(dim(), key());
  return out;
}

namespace {
template <class Op, class Groups>
void reduce_(Op op, const Dim reductionDim, const Variable &out_data,
             const DataArray &data, const Dim dim, const Groups &groups,
             const FillValue fill) {
  const auto mask_replacement =
      special_like(Variable(data.data(), Dimensions{}), fill);
  auto mask = irreducible_mask(data.masks(), reductionDim);
  const auto process = [&](const auto &range) {
    // Apply to each group, storing result in output slice
    for (scipp::index group = range.begin(); group != range.end(); ++group) {
      auto out_slice = out_data.slice({dim, group});
      for (const auto &slice : groups[group]) {
        const auto data_slice = data.data().slice(slice);
        if (mask.is_valid())
          op(out_slice, where(mask.slice(slice), mask_replacement, data_slice));
        else
          op(out_slice, data_slice);
      }
    }
  };
  core::parallel::parallel_for(core::parallel::blocked_range(0, groups.size()),
                               process);
}
} // namespace

template <class T>
template <class Op>
T GroupBy<T>::reduce(Op op, const Dim reductionDim,
                     const FillValue fill) const {
  auto out = makeReductionOutput(reductionDim, fill);
  if constexpr (std::is_same_v<T, Dataset>) {
    for (const auto &item : m_data)
      reduce_(op, reductionDim, out[item.name()].data(), item, dim(), groups(),
              fill);
  } else {
    reduce_(op, reductionDim, out.data(), m_data, dim(), groups(), fill);
  }
  return out;
}

/// Reduce each group by concatenating elements and return combined data.
///
/// This only supports binned data.
template <class T> T GroupBy<T>::concat(const Dim reductionDim) const {
  const auto conc = [&](const auto &data) {
    if (key().dims().volume() == scipp::size(groups()))
      return groupby_concat_bins(data, {}, key(), reductionDim);
    else
      return groupby_concat_bins(data, key(), {}, reductionDim);
  };
  if constexpr (std::is_same_v<T, DataArray>) {
    return conc(m_data);
  } else {
    return apply_to_items(m_data, [&](auto &&..._) { return conc(_...); });
  }
}

/// Reduce each group using `sum` and return combined data.
template <class T> T GroupBy<T>::sum(const Dim reductionDim) const {
  return reduce(variable::sum_into, reductionDim, FillValue::ZeroNotBool);
}

/// Reduce each group using `nansum` and return combined data.
template <class T> T GroupBy<T>::nansum(const Dim reductionDim) const {
  return reduce(variable::nansum_into, reductionDim, FillValue::ZeroNotBool);
}

/// Reduce each group using `all` and return combined data.
template <class T> T GroupBy<T>::all(const Dim reductionDim) const {
  return reduce(variable::all_into, reductionDim, FillValue::True);
}

/// Reduce each group using `any` and return combined data.
template <class T> T GroupBy<T>::any(const Dim reductionDim) const {
  return reduce(variable::any_into, reductionDim, FillValue::False);
}

/// Reduce each group using `max` and return combined data.
template <class T> T GroupBy<T>::max(const Dim reductionDim) const {
  return reduce(variable::max_into, reductionDim, FillValue::Lowest);
}

/// Reduce each group using `nanmax` and return combined data.
template <class T> T GroupBy<T>::nanmax(const Dim reductionDim) const {
  return reduce(variable::nanmax_into, reductionDim, FillValue::Lowest);
}

/// Reduce each group using `min` and return combined data.
template <class T> T GroupBy<T>::min(const Dim reductionDim) const {
  return reduce(variable::min_into, reductionDim, FillValue::Max);
}

/// Reduce each group using `nanmin` and return combined data.
template <class T> T GroupBy<T>::nanmin(const Dim reductionDim) const {
  return reduce(variable::nanmin_into, reductionDim, FillValue::Max);
}

/// Apply mean to groups and return combined data.
template <class T> T GroupBy<T>::mean(const Dim reductionDim) const {
  // 1. Sum into output slices
  auto out = sum(reductionDim);

  // 2. Compute number of slices N contributing to each out slice
  const auto get_scale = [&](const auto &data) {
    // TODO Supporting binned data requires generalized approach to compute
    // scale factor.
    if (is_bins(data))
      throw except::NotImplementedError(
          "groupby.mean does not support binned data yet.");
    auto scale = makeVariable<double>(Dims{dim()}, Shape{size()});
    const auto scaleT = scale.template values<double>();
    const auto mask = irreducible_mask(data.masks(), reductionDim);
    for (scipp::index group = 0; group < size(); ++group)
      for (const auto &slice : groups()[group]) {
        // N contributing to each slice
        scaleT[group] += slice.end() - slice.begin();
        // N masks for each slice, that need to be subtracted
        if (mask.is_valid()) {
          const auto masks_sum = variable::sum(mask.slice(slice), reductionDim);
          scaleT[group] -= masks_sum.template value<int64_t>();
        }
      }
    return reciprocal(std::move(scale));
  };

  // 3. sum/N -> mean
  if constexpr (std::is_same_v<T, Dataset>) {
    for (auto &&item : out) {
      if (is_int(item.data().dtype()))
        out.setData(item.name(), item.data() * get_scale(m_data[item.name()]));
      else
        item *= get_scale(m_data[item.name()]);
    }
  } else {
    if (is_int(out.data().dtype()))
      out.setData(out.data() * get_scale(m_data));
    else
      out *= get_scale(m_data);
  }

  return out;
}

namespace {
template <class T> struct NanSensitiveLess {
  // Compare two values such that x < NaN for all x != NaN.
  // Note: if changing this in future, ensure it meets the requirements from
  // https://en.cppreference.com/w/cpp/named_req/Compare, as it is used as
  // the comparator for keys in a map.
  bool operator()(const T &a, const T &b) const {
    if constexpr (std::is_floating_point_v<T>)
      if (std::isnan(b))
        return !std::isnan(a);
    return a < b;
  }
};

template <class T> struct nan_sensitive_equal {
  bool operator()(const T &a, const T &b) const {
    if constexpr (std::is_floating_point_v<T>)
      return a == b || (std::isnan(a) && std::isnan(b));
    else
      return a == b;
  }
};
} // namespace

template <class T> struct MakeGroups {
  static GroupByGrouping apply(const Variable &key, const Dim targetDim) {
    expect::is_key(key);
    const auto &values = key.values<T>();

    const auto dim = key.dim();
    std::unordered_map<T, GroupByGrouping::group, std::hash<T>,
                       nan_sensitive_equal<T>>
        indices;
    const auto end = values.end();
    scipp::index i = 0;
    for (auto it = values.begin(); it != end;) {
      // Use contiguous (thick) slices if possible to avoid overhead of slice
      // handling in follow-up "apply" steps.
      const auto begin = i;
      const auto &group_value = *it;
      while (it != end && nan_sensitive_equal<T>()(*it, group_value)) {
        ++it;
        ++i;
      }
      indices[group_value].emplace_back(dim, begin, i);
    }

    std::vector<T> keys;
    std::vector<GroupByGrouping::group> groups;
    keys.reserve(scipp::size(indices));
    groups.reserve(scipp::size(indices));
    for (auto &item : indices)
      keys.emplace_back(item.first);
    core::parallel::parallel_sort(keys.begin(), keys.end(),
                                  NanSensitiveLess<T>());
    for (const auto &k : keys) {
      // false positive, fixed in https://github.com/danmar/cppcheck/pull/4230
      // cppcheck-suppress containerOutOfBounds
      groups.emplace_back(std::move(indices.at(k)));
    }

    const Dimensions dims{targetDim, scipp::size(indices)};
    auto keys_ = makeVariable<T>(Dimensions{dims}, Values(std::move(keys)));
    keys_.setUnit(key.unit());
    return {dim, std::move(keys_), std::move(groups)};
  }
};

template <class T> struct MakeBinGroups {
  static GroupByGrouping apply(const Variable &key, const Variable &bins) {
    expect::is_key(key);
    if (bins.dims().ndim() != 1)
      throw except::DimensionError("Group-by bins must be 1-dimensional");
    if (key.unit() != bins.unit())
      throw except::UnitError("Group-by key must have same unit as bins");
    const auto &values = key.values<T>();
    const auto &edges = bins.values<T>();
    core::expect::histogram::sorted_edges(edges);

    const auto dim = key.dim();
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
    return {dim, bins, std::move(groups)};
  }
};

template <class T>
GroupBy<T> call_groupby(const T &array, const Variable &key,
                        const Variable &bins) {
  return {
      array,
      core::CallDType<double, float, int64_t, int32_t>::apply<MakeBinGroups>(
          key.dtype(), key, bins)};
}

template <class T>
GroupBy<T> call_groupby(const T &array, const Variable &key, const Dim &dim) {
  return {array,
          core::CallDType<double, float, int64_t, int32_t, bool, std::string,
                          core::time_point>::apply<MakeGroups>(key.dtype(), key,
                                                               dim)};
}

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by a coord.
/// Grouping will create a new coordinate for the dimension of the grouping
/// coord in a later apply/combine step.
GroupBy<DataArray> groupby(const DataArray &array, const Dim dim) {
  const auto &key = array.coords()[dim];
  return call_groupby(array, key, dim);
}

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<DataArray> groupby(const DataArray &array, const Dim dim,
                           const Variable &bins) {
  const auto &key = array.coords()[dim];
  return groupby(array, key, bins);
}

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<DataArray> groupby(const DataArray &array, const Variable &key,
                           const Variable &bins) {
  if (!array.dims().includes(key.dims()))
    throw except::DimensionError("Size of Group-by key is incorrect.");

  return call_groupby(array, key, bins);
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by a coord.
/// Grouping will create a new coordinate for the dimension of the grouping
/// coord in a later apply/combine step.
GroupBy<Dataset> groupby(const Dataset &dataset, const Dim dim) {
  const auto &key = dataset.coords()[dim];
  return call_groupby(dataset, key, dim);
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<Dataset> groupby(const Dataset &dataset, const Dim dim,
                         const Variable &bins) {
  const auto &key = dataset.coords()[dim];
  return groupby(dataset, key, bins);
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<Dataset> groupby(const Dataset &dataset, const Variable &key,
                         const Variable &bins) {
  for (const auto &dim : dataset.sizes()) {
    Dimensions dims(dim, dataset.sizes()[dim]);
    if (dims.includes(key.dims()))
      // Found compatible Dimension.
      return call_groupby(dataset, key, bins);
  }
  // No Dimension contains the key - throw.
  throw except::DimensionError("Size of Group-by key is incorrect.");
}

template class GroupBy<DataArray>;
template class GroupBy<Dataset>;

} // namespace scipp::dataset
