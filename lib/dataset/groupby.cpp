// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
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
#include "scipp/variable/operations.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/choose.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/shape.h"

#include "../variable/operations_common.h"
#include "bin_common.h"
#include "dataset_operations_common.h"

using namespace scipp::variable;

namespace scipp::dataset {

namespace {

template <class Slices, class Data>
auto copy_impl(const Slices &slices, const Data &data, const Dim slice_dim,
               const AttrPolicy attrPolicy = AttrPolicy::Keep) {
  scipp::index size = 0;
  for (const auto &slice : slices)
    size += slice.end() - slice.begin();
  auto out = dataset::copy(data.slice({slice_dim, 0, size}), attrPolicy);
  scipp::index current = 0;
  auto out_slices = slices;
  for (auto &slice : out_slices) {
    const auto thickness = slice.end() - slice.begin();
    slice = Slice(slice.dim(), current, current + thickness);
    current += thickness;
  }
  const auto copy_slice = [&](const auto &range) {
    for (scipp::index i = range.begin(); i != range.end(); ++i) {
      const auto &slice = slices[i];
      const auto &out_slice = out_slices[i];
      scipp::dataset::copy(
          strip_if_broadcast_along(data.slice(slice), slice_dim),
          out.slice(out_slice), attrPolicy);
    }
  };
  core::parallel::parallel_for(core::parallel::blocked_range(0, slices.size()),
                               copy_slice);
  return out;
}

} // namespace

/// Extract given group as a new data array or dataset
template <class T>
T GroupBy<T>::copy(const scipp::index group,
                   const AttrPolicy attrPolicy) const {
  return copy_impl(groups().at(group),
                   strip_edges_along(m_data, m_grouping.sliceDim()),
                   m_grouping.sliceDim(), attrPolicy);
}

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
  out.rename(reductionDim, dim());
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
    return apply_to_items(m_data, [&](auto &&... _) { return conc(_...); });
  }
}

/// Reduce each group using `sum` and return combined data.
template <class T> T GroupBy<T>::sum(const Dim reductionDim) const {
  return reduce(sum_into, reductionDim, FillValue::ZeroNotBool);
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

/// Reduce each group using `min` and return combined data.
template <class T> T GroupBy<T>::min(const Dim reductionDim) const {
  return reduce(variable::min_into, reductionDim, FillValue::Max);
}

/// Combine groups without changes, effectively sorting data.
template <class T> T GroupBy<T>::copy(const SortOrder order) const {
  std::vector<Slice> flat;
  if (order == SortOrder::Ascending)
    for (const auto &slices : groups())
      flat.insert(flat.end(), slices.begin(), slices.end());
  else
    for (auto it = groups().rbegin(); it != groups().rend(); ++it)
      flat.insert(flat.end(), it->begin(), it->end());
  return copy_impl(flat, m_data, m_grouping.sliceDim());
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
        out.setData(item.name(), item.data() * get_scale(m_data[item.name()]),
                    AttrPolicy::Keep);
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

template <class T> bool nan_sensitive_equal(const T &a, const T &b) {
  if constexpr (std::is_floating_point_v<T>)
    return a == b || (std::isnan(a) && std::isnan(b));
  else
    return a == b;
}
} // namespace

template <class T> struct MakeGroups {
  static auto apply(const Variable &key, const Dim targetDim) {
    expect::is_key(key);
    const auto &values = key.values<T>();

    const auto dim = key.dim();
    std::map<T, GroupByGrouping::group, NanSensitiveLess<T>> indices;
    const auto end = values.end();
    scipp::index i = 0;
    for (auto it = values.begin(); it != end;) {
      // Use contiguous (thick) slices if possible to avoid overhead of slice
      // handling in follow-up "apply" steps.
      const auto begin = i;
      const auto &group_value = *it;
      while (it != end && nan_sensitive_equal(*it, group_value)) {
        ++it;
        ++i;
      }
      indices[group_value].emplace_back(dim, begin, i);
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
    return GroupByGrouping{dim, std::move(keys_), std::move(groups)};
  }
};

template <class T> struct MakeBinGroups {
  static auto apply(const Variable &key, const Variable &bins) {
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
    return GroupByGrouping{dim, bins, std::move(groups)};
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
  const auto &key = array.meta()[dim];
  return call_groupby(array, key, dim);
}

/// Create GroupBy<DataArray> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `array` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<DataArray> groupby(const DataArray &array, const Dim dim,
                           const Variable &bins) {
  const auto &key = array.meta()[dim];
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
  const auto &key = dataset.meta()[dim];
  return call_groupby(dataset, key, dim);
}

/// Create GroupBy<Dataset> object as part of "split-apply-combine" mechanism.
///
/// Groups the slices of `dataset` according to values in given by a coord.
/// Grouping of a coord is according to given `bins`, which will be added as a
/// new coordinate to the output in a later apply/combine step.
GroupBy<Dataset> groupby(const Dataset &dataset, const Dim dim,
                         const Variable &bins) {
  const auto &key = dataset.meta()[dim];
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

Variable extract(const Variable &var, const Variable &condition) {
  return extract(DataArray(var), condition).data();
}

namespace {
template <class T> T extract_impl(const T &obj, const Variable &condition) {
  if (condition.dtype() != dtype<bool>)
    throw except::TypeError(
        "Cannot extract elements based on condition with non-boolean dtype. If "
        "you intended to select a range based on a label you must specify the "
        "dimension.");
  if (all(condition).value<bool>())
    return copy(obj);
  if (!any(condition).value<bool>())
    return copy(obj.slice({condition.dim(), 0, 0}));
  return call_groupby(obj, condition, condition.dim()).copy(1);
}
} // namespace

DataArray extract(const DataArray &da, const Variable &condition) {
  return extract_impl(da, condition);
}

Dataset extract(const Dataset &ds, const Variable &condition) {
  return extract_impl(ds, condition);
}

template class GroupBy<DataArray>;
template class GroupBy<Dataset>;

constexpr auto slice_by_value = [](const auto &x, const Dim dim,
                                   const auto &key) {
  const auto size = x.dims()[dim];
  const auto &coord = x.meta()[dim];
  for (scipp::index i = 0; i < size; ++i)
    if (coord.slice({dim, i}) == key)
      return x.slice({dim, i});
  throw std::runtime_error("Given key not found in coord.");
};

/// Similar to numpy.choose, but choose based on *values* in `key`.
///
/// Chooses slices of `choices` along `dim`, based on values of dimension-coord
/// for `dim`.
DataArray choose(const Variable &key, const DataArray &choices, const Dim dim) {
  const auto grouping = call_groupby(key, key, dim);
  const Dim target_dim = key.dims().inner();
  auto out = resize(choices, dim, key.dims()[target_dim]);
  out.rename(dim, target_dim);
  out.coords().set(dim, key); // not target_dim
  for (scipp::index group = 0; group < grouping.size(); ++group) {
    const auto value = grouping.key().slice({dim, group});
    const auto &choice = slice_by_value(choices, dim, value);
    for (const auto &slice : grouping.groups()[group]) {
      auto out_ = out.slice(slice);
      copy(broadcast(choice.data(), out_.dims()), out_.data());
    }
  }
  return out;
}

} // namespace scipp::dataset
