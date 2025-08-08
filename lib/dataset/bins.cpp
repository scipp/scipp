// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <limits>

#include "scipp/core/bucket.h"
#include "scipp/core/element/event_operations.h"
#include "scipp/core/element/histogram.h"
#include "scipp/core/except.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/transform_subspan.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"

#include "../variable/operations_common.h"
#include "bin_common.h"
#include "bin_detail.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {
namespace {
constexpr auto copy_or_match = [](const auto &a, auto &&b, const Dim dim,
                                  const Variable &srcIndices,
                                  const Variable &dstIndices) {
  if (a.dims().contains(dim))
    copy_slices(a, b, dim, srcIndices, dstIndices);
  else
    core::expect::equals(a, b);
};

constexpr auto expect_matching_keys = [](const auto &a, const auto &b) {
  bool ok = true;
  constexpr auto key = [](const auto &x_) {
    if constexpr (std::is_base_of_v<DataArray, std::decay_t<decltype(x_)>>)
      return x_.name();
    else
      return x_.first;
  };
  for (const auto &x : a)
    ok &= b.contains(key(x));
  for (const auto &x : b)
    ok &= a.contains(key(x));
  if (!ok)
    throw std::runtime_error("Mismatching keys in\n" + to_string(a) + " and\n" +
                             to_string(b));
};

auto make_fill(const DataArray &function,
               const std::optional<Variable> &fill_value) {
  Variable fill = fill_value.value_or(zero_like(function.data()));
  if (fill_value) {
    if (fill.dtype() != function.dtype())
      throw except::TypeError(
          "The fill_value (dtype=" + to_string(fill.dtype()) +
          ") must have the same dtype as the function values (dtype=" +
          to_string(function.dtype()) + ").");
  } else if (fill.dtype() == dtype<double>) {
    fill.value<double>() = std::numeric_limits<double>::quiet_NaN();
  } else if (fill.dtype() == dtype<float>) {
    fill.value<float>() = std::numeric_limits<float>::quiet_NaN();
  }
  return fill;
}

} // namespace

void copy_slices(const DataArray &src, DataArray dst, const Dim dim,
                 const Variable &srcIndices, const Variable &dstIndices) {
  copy_slices(src.data(), dst.data(), dim, srcIndices, dstIndices);
  expect_matching_keys(src.coords(), dst.coords());
  expect_matching_keys(src.masks(), dst.masks());
  for (const auto &[name, coord] : src.coords())
    copy_or_match(coord, dst.coords()[name], dim, srcIndices, dstIndices);
  for (const auto &[name, mask] : src.masks())
    copy_or_match(mask, dst.masks()[name], dim, srcIndices, dstIndices);
}

void copy_slices(const Dataset &src, Dataset dst, const Dim dim,
                 const Variable &srcIndices, const Variable &dstIndices) {
  for (const auto &[name, var] : src.coords())
    copy_or_match(var, dst.coords()[name], dim, srcIndices, dstIndices);
  expect_matching_keys(src.coords(), dst.coords());
  expect_matching_keys(src, dst);
  for (const auto &item : src) {
    const auto &dst_ = dst[item.name()];
    expect_matching_keys(item.masks(), dst_.masks());
    copy_or_match(item.data(), dst_.data(), dim, srcIndices, dstIndices);
    for (const auto &[name, var] : item.masks())
      copy_or_match(var, dst_.masks()[name], dim, srcIndices, dstIndices);
  }
}

namespace {
constexpr auto copy_or_resize = [](const auto &var, const Dim dim,
                                   const scipp::index size) {
  auto dims = var.dims();
  if (dims.contains(dim))
    dims.resize(dim, size);
  // Using variableFactory instead of variable::resize for creating
  // _uninitialized_ variable.
  return var.dims().contains(dim)
             ? variable::variableFactory().create(var.dtype(), dims, var.unit(),
                                                  var.has_variances())
             : copy(var);
};
} // namespace

// TODO These functions are an unfortunate near-duplicate of `resize`. However,
// the latter drops coords along the resized dimension. Is there a way to unify
// this? Can the need to drop coords in resize be avoided?
DataArray resize_default_init(const DataArray &parent, const Dim dim,
                              const scipp::index size) {
  DataArray buffer(copy_or_resize(parent.data(), dim, size));
  for (const auto &[name, var] : parent.coords())
    buffer.coords().set(name, copy_or_resize(var, dim, size));
  for (const auto &[name, var] : parent.masks())
    buffer.masks().set(name, copy_or_resize(var, dim, size));
  return buffer;
}

Dataset resize_default_init(const Dataset &parent, const Dim dim,
                            const scipp::index size) {
  auto new_sizes = parent.sizes();
  if (new_sizes.contains(dim))
    new_sizes.resize(dim, size);

  Dataset buffer({}, Coords(new_sizes, {}));
  for (const auto &[name, var] : parent.coords())
    buffer.setCoord(name, copy_or_resize(var, dim, size));
  for (const auto &item : parent) {
    buffer.setData(item.name(), copy_or_resize(item.data(), dim, size));
    for (const auto &[name, var] : item.masks())
      buffer[item.name()].masks().set(name, copy_or_resize(var, dim, size));
  }
  return buffer;
}

/// Construct a bin-variable over a data array.
///
/// Each bin is represented by a Variable slice. `indices` defines the array of
/// bins as slices of `buffer` along `dim`.
Variable make_bins(Variable indices, const Dim dim, DataArray buffer) {
  expect_valid_bin_indices(indices, dim, buffer.dims());
  return make_bins_no_validate(std::move(indices), dim, std::move(buffer));
}

/// Construct a bin-variable over a data array without index validation.
///
/// Must be used only when it is guaranteed that indices are valid or overlap of
/// bins is acceptable.
Variable make_bins_no_validate(Variable indices, const Dim dim,
                               DataArray buffer) {
  return variable::make_bins_impl(std::move(indices), dim, std::move(buffer));
}

/// Construct a bin-variable over a dataset.
///
/// Each bin is represented by a Variable slice. `indices` defines the array of
/// bins as slices of `buffer` along `dim`.
Variable make_bins(Variable indices, const Dim dim, Dataset buffer) {
  expect_valid_bin_indices(indices, dim, buffer.sizes());
  return make_bins_no_validate(std::move(indices), dim, std::move(buffer));
}

/// Construct a bin-variable over a dataset without index validation.
///
/// Must be used only when it is guaranteed that indices are valid or overlap of
/// bins is acceptable.
Variable make_bins_no_validate(Variable indices, const Dim dim,
                               Dataset buffer) {
  return variable::make_bins_impl(std::move(indices), dim, std::move(buffer));
}

bool is_bins(const DataArray &array) { return is_bins(array.data()); }

bool is_bins(const Dataset &dataset) {
  return std::any_of(dataset.begin(), dataset.end(),
                     [](const auto &item) { return is_bins(item); });
}

Variable lookup_previous(const DataArray &function, const Variable &x, Dim dim,
                         const std::optional<Variable> &fill_value) {
  const auto fill = make_fill(function, fill_value);
  const auto &coord = function.coords()[dim];
  const auto data = masked_data(function, dim, fill);
  const auto weights = subspan_view(data, dim);
  if (!allsorted(coord, dim))
    throw except::DataArrayError(
        "Coordinate of lookup function must be sorted.");
  // Note that we could do a linspace optimization similar to buckets::map here.
  // Add this if we have real world application that would benefit.
  return variable::transform(x, subspan_view(coord, dim), weights, fill,
                             core::element::event::lookup_previous,
                             "lookup_previous");
}

Variable pretend_bins_for_threading(const DataArray &da, Dim bin_dim) {
  const auto dim = da.dims().inner();
  const auto size = std::max(scipp::index(1), da.dims()[dim]);
  const auto nthread = size > 8000000   ? 24
                       : size > 4000000 ? 16
                       : size > 1000000 ? 8
                       : size > 200000  ? 4
                       : size > 100000  ? 2
                                        : 1;

  const auto stride = std::max(scipp::index(1), size / nthread);
  auto begin = bin_detail::make_range(0, size, stride, bin_dim);
  auto end = begin + stride * sc_units::none;
  end.values<scipp::index>().as_span().back() = da.dims()[dim];
  const auto indices = zip(begin, end);
  return make_bins_no_validate(indices, dim, da);
}

} // namespace scipp::dataset

namespace scipp::dataset::buckets {
namespace {

template <class T> auto combine(const Variable &var0, const Variable &var1) {
  const auto &[indices0, dim0, buffer0] = var0.constituents<T>();
  const auto &[indices1, dim1, buffer1] = var1.constituents<T>();
  static_cast<void>(buffer1);
  static_cast<void>(dim1);
  const Dim dim = dim0;
  const auto [begin0, end0] = unzip(indices0);
  const auto [begin1, end1] = unzip(indices1);
  const auto sizes0 = end0 - begin0;
  const auto sizes1 = end1 - begin1;
  const auto sizes = sizes0 + sizes1;
  const auto end = cumsum(sizes);
  const auto begin = end - sizes;
  const auto total_size =
      end.dims().volume() > 0
          ? end.template values<scipp::index>().as_span().back()
          : 0;
  auto buffer = resize_default_init(buffer0, dim, total_size);
  copy_slices(buffer0, buffer, dim, indices0, zip(begin, end - sizes1));
  copy_slices(buffer1, buffer, dim, indices1, zip(begin + sizes0, end));
  return make_bins_no_validate(zip(begin, end), dim, std::move(buffer));
}

template <class T>
auto concatenate_impl(const Variable &var0, const Variable &var1) {
  return combine<T>(var0, var1);
}

} // namespace

Variable concatenate(const Variable &var0, const Variable &var1) {
  if (var0.dtype() == dtype<bucket<Variable>>)
    return concatenate_impl<Variable>(var0, var1);
  else if (var0.dtype() == dtype<bucket<DataArray>>)
    return concatenate_impl<DataArray>(var0, var1);
  else
    return concatenate_impl<Dataset>(var0, var1);
}

DataArray concatenate(const DataArray &a, const DataArray &b) {
  return DataArray{buckets::concatenate(a.data(), b.data()),
                   union_(a.coords(), b.coords(), "concatenate"),
                   union_or(a.masks(), b.masks())};
}

/// Reduce a dimension by concatenating all elements along the dimension.
///
/// This is the analogue to summing non-bucket data.
Variable concatenate(const Variable &var, const Dim dim) {
  if (var.dtype() == dtype<bucket<Variable>>)
    return concat_bins<Variable>(var, dim);
  else
    return concat_bins<DataArray>(var, dim);
}

/// Reduce a dimension by concatenating all elements along the dimension.
///
/// This is the analogue to summing non-bucket data.
DataArray concatenate(const DataArray &array, const Dim dim) {
  return groupby_concat_bins(array, {}, {}, {dim});
}

void append(Variable &var0, const Variable &var1) {
  if (var0.dtype() == dtype<bucket<Variable>>)
    var0.setDataHandle(combine<Variable>(var0, var1).data_handle());
  else if (var0.dtype() == dtype<bucket<DataArray>>)
    var0.setDataHandle(combine<DataArray>(var0, var1).data_handle());
  else
    var0.setDataHandle(combine<Dataset>(var0, var1).data_handle());
}

void append(Variable &&var0, const Variable &var1) { append(var0, var1); }

void append(DataArray &a, const DataArray &b) {
  expect::coords_are_superset(a, b, "bins.append");
  union_or_in_place(a.masks(), b.masks());
  auto data = a.data();
  append(data, b.data());
  a.setData(data);
}

Variable histogram(const Variable &data, const Variable &binEdges) {
  using namespace scipp::core;
  auto hist_dim = binEdges.dims().inner();
  auto &&[indices, dim, buffer] = data.constituents<DataArray>();
  // `hist_dim` may be the same as a dim of data if there is existing binning.
  // We rename to a dummy to avoid duplicate dimensions, perform histogramming,
  // and then sum over the dummy dimensions, i.e., sum contributions from all
  // inputs bins to the same output histogram. This also allows for threading of
  // 1-D histogramming provided that the input has multiple bins along
  // `hist_dim`.
  const Dim dummy = Dim::InternalHistogram;
  const auto nbin = binEdges.dims()[hist_dim] - 1;
  if (indices.dims().contains(hist_dim)) {
    // With large existing dim matching the new dim, we would create a large
    // intermediate histogrammed result, which leads to performance and memory
    // issues. This is a suboptimal (since it concatenates first) but simple way
    // to avoid the problem.
    if (indices.dims().volume() * nbin > 100000000) { // about 1 GByte
      const auto tmp = concatenate(data, hist_dim);
      if (tmp.ndim() == 0) // Operate on buffer so we get multi-threading
        return histogram(tmp.bin_buffer<DataArray>(), binEdges).data();
      else
        return histogram(tmp, binEdges);
    }
    indices = indices.rename_dims({{hist_dim, dummy}});
  }

  const auto masked = masked_data(buffer, dim);
  const auto coord = buffer.coords()[hist_dim];
  const auto dt = common_type(binEdges, coord);
  const auto promoted_coord = astype(coord, dt, CopyPolicy::TryAvoid);
  const auto promoted_edges = astype(binEdges, dt, CopyPolicy::TryAvoid);
  auto hist = variable::transform_subspan(
      buffer.dtype(), hist_dim, nbin,
      subspan_view(promoted_coord, dim, indices),
      subspan_view(masked, dim, indices), promoted_edges, element::histogram,
      "histogram");
  if (hist.dims().contains(dummy))
    return sum(hist, dummy);
  else
    return hist;
}

Variable map(const DataArray &function, const Variable &x, Dim dim,
             const std::optional<Variable> &fill_value) {
  const auto fill = make_fill(function, fill_value);
  if (dim == Dim::Invalid)
    dim = edge_dimension(function);
  const auto &edges = function.coords()[dim];
  if (!is_edges(function.dims(), edges.dims(), dim))
    throw except::BinEdgeError(
        "Function used as lookup table in map operation must be a histogram");
  const auto data = masked_data(function, dim, fill);
  const auto weights = subspan_view(data, dim);
  if (all(islinspace(edges, dim)).value<bool>()) {
    return variable::transform(x, subspan_view(edges, dim), weights, fill,
                               core::element::event::map_linspace, "map");
  } else {
    if (!allsorted(edges, dim))
      throw except::BinEdgeError("Bin edges of histogram must be sorted.");
    return variable::transform(x, subspan_view(edges, dim), weights, fill,
                               core::element::event::map_sorted_edges, "map");
  }
}

namespace {
Masks masks_not_in_dim(const Masks &all_masks, const Dim dim) {
  Masks results;
  for (auto [name, mask] : all_masks) {
    if (!mask.dims().contains(dim)) {
      results.set(name, mask);
    }
  }
  return results;
}
} // namespace

void scale(DataArray &array, const DataArray &histogram, Dim dim) {
  if (dim == Dim::Invalid)
    dim = edge_dimension(histogram);
  // Coords along dim are ignored since "binning" is dynamic for buckets.
  expect::coords_are_superset(array, histogram.slice({dim, 0}), "bins.scale");
  // scale applies masks along dim but others are kept
  union_or_in_place(array.masks(), masks_not_in_dim(histogram.masks(), dim));
  auto data = bins_view<DataArray>(array.data()).data();
  const auto &coord = bins_view<DataArray>(array.data()).coords()[dim];
  const auto &edges = histogram.coords()[dim];
  const auto masked = masked_data(histogram, dim);
  const auto weights = subspan_view(masked, dim);
  if (all(islinspace(edges, dim)).value<bool>()) {
    transform_in_place(data, coord, subspan_view(edges, dim), weights,
                       core::element::event::map_and_mul_linspace,
                       "bins.scale");
  } else {
    if (!allsorted(edges, dim))
      throw except::BinEdgeError("Bin edges of histogram must be sorted.");
    transform_in_place(data, coord, subspan_view(edges, dim), weights,
                       core::element::event::map_and_mul_sorted_edges,
                       "bins.scale");
  }
}
} // namespace scipp::dataset::buckets
