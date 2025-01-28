// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/extract.h"
#include "scipp/dataset/util.h"

namespace scipp {

namespace {

/// Transform data of data array or dataset, coord and masks are shallow-copied.
///
/// Beware of the mask-copy behavior, which is not suitable for data returned to
/// the user.
template <class T, class Func, class... Ts>
T transform_data(const T &obj, Func func, const Ts &...other) {
  T out(obj);
  if constexpr (std::is_same_v<T, Variable>) {
    return func(obj, other...);
  } else if constexpr (std::is_same_v<T, DataArray>) {
    out.setData(func(obj.data(), other.data()...));
  } else {
    for (const auto &item : obj)
      out.setData(item.name(), func(item.data(), other[item.name()].data()...));
  }
  return out;
}

template <class Buffer>
Variable copy_ranges_from_buffer(const Variable &indices, const Dim dim,
                                 const Buffer &buffer) {
  return copy(make_bins_no_validate(indices, dim, buffer));
}

Variable copy_ranges_from_bins_buffer(const Variable &indices,
                                      const Variable &data) {
  if (data.dtype() == dtype<bucket<Variable>>) {
    const auto &[i, dim, buf] = data.constituents<Variable>();
    return copy_ranges_from_buffer(indices, dim, buf);
  } else if (data.dtype() == dtype<bucket<DataArray>>) {
    const auto &[i, dim, buf] = data.constituents<DataArray>();
    return copy_ranges_from_buffer(indices, dim, buf);
  } else {
    const auto &[i, dim, buf] = data.constituents<Dataset>();
    return copy_ranges_from_buffer(indices, dim, buf);
  }
}

Variable dense_or_bin_indices(const Variable &var) {
  return is_bins(var) ? var.bin_indices() : var;
}

Variable dense_or_copy_bin_elements(const Variable &dense_or_indices,
                                    const Variable &data) {
  return is_bins(data) ? copy_ranges_from_bins_buffer(dense_or_indices, data)
                       : dense_or_indices;
}
} // namespace

template <class T>
T extract_ranges(const Variable &indices, const T &data, const Dim dim) {
  T no_edges;
  if constexpr (std::is_same_v<T, Variable>)
    no_edges = data;
  else
    no_edges = strip_edges_along(data, dim);
  // 1. Operate on dense data, or equivalent array of indices (if binned) to
  // obtain output data of correct shape with proper meta data.
  auto dense = transform_data(no_edges, dense_or_bin_indices);
  auto out =
      copy_ranges_from_buffer(indices, dim, dense).template bin_buffer<T>();
  // 2. If we have binned data then the data of the DataArray or Dataset
  // obtained in step 1. give the indices into the underlying buffer to be
  // copied. This then replaces the data to obtain the final result. Does
  // nothing if dense data.
  return transform_data(out, dense_or_copy_bin_elements, no_edges);
}

namespace {
template <class T> T extract_impl(const T &obj, const Variable &condition) {
  if (condition.dtype() != dtype<bool>)
    throw except::TypeError(
        "Cannot extract elements based on condition with non-boolean dtype. If "
        "you intended to select a range based on a label you must specify the "
        "dimension.");
  if (condition.dims().ndim() != 1)
    throw except::DimensionError("Condition must by 1-D, but got " +
                                 to_string(condition.dims()) + '.');
  if (!obj.dims().includes(condition.dims()))
    throw except::DimensionError(
        "Condition dimensions " + to_string(condition.dims()) +
        " must be be included in the dimensions of the sliced object " +
        to_string(obj.dims()) + '.');

  auto values = condition.values<bool>().as_span();
  std::vector<scipp::index_pair> indices;
  for (scipp::index i = 0; i < scipp::size(values); ++i) {
    if (i > 0 && values[i - 1] == values[i])
      continue;    // not an edge
    if (values[i]) // rising edge
      indices.emplace_back(i, scipp::size(values));
    else if (i != 0) // falling edge
      indices.back().second = i;
  }
  return extract_ranges(makeVariable<scipp::index_pair>(Dims{condition.dim()},
                                                        Shape{indices.size()},
                                                        Values(indices)),
                        obj, condition.dim());
}
} // namespace

Variable extract(const Variable &var, const Variable &condition) {
  return extract_impl(var, condition);
}

DataArray extract(const DataArray &da, const Variable &condition) {
  return extract_impl(da, condition);
}

Dataset extract(const Dataset &ds, const Variable &condition) {
  return extract_impl(ds, condition);
}

template SCIPP_DATASET_EXPORT Variable extract_ranges(const Variable &,
                                                      const Variable &,
                                                      const Dim);
template SCIPP_DATASET_EXPORT DataArray extract_ranges(const Variable &,
                                                       const DataArray &,
                                                       const Dim);
template SCIPP_DATASET_EXPORT Dataset extract_ranges(const Variable &,
                                                     const Dataset &,
                                                     const Dim);

} // namespace scipp
