// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/common/overloaded.h"
#include "scipp/core/bucket.h"
#include "scipp/core/element/event_operations.h"
#include "scipp/core/element/histogram.h"
#include "scipp/core/except.h"
#include "scipp/core/histogram.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/transform_subspan.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/reduction.h"
#include "scipp/dataset/shape.h"

#include "../variable/operations_common.h"
#include "bin_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {
namespace {
constexpr auto copy_or_match = [](const auto &a, const auto &b, const Dim dim,
                                  const VariableConstView &srcIndices,
                                  const VariableConstView &dstIndices) {
  if (a.dims().contains(dim))
    copy_slices(a, b, dim, srcIndices, dstIndices);
  else
    core::expect::equals(a, b);
};
} // namespace

void copy_slices(const DataArrayConstView &src, const DataArrayView &dst,
                 const Dim dim, const VariableConstView &srcIndices,
                 const VariableConstView &dstIndices) {
  copy_slices(src.data(), dst.data(), dim, srcIndices, dstIndices);
  core::expect::sizeMatches(src.meta(), dst.meta());
  core::expect::sizeMatches(src.masks(), dst.masks());
  for (const auto &[name, coord] : src.meta())
    copy_or_match(coord, dst.meta()[name], dim, srcIndices, dstIndices);
  for (const auto &[name, mask] : src.masks())
    copy_or_match(mask, dst.masks()[name], dim, srcIndices, dstIndices);
}

void copy_slices(const DatasetConstView &src, const DatasetView &dst,
                 const Dim dim, const VariableConstView &srcIndices,
                 const VariableConstView &dstIndices) {
  for (const auto &[name, var] : src.coords())
    copy_or_match(var, dst.coords()[name], dim, srcIndices, dstIndices);
  core::expect::sizeMatches(src.coords(), dst.coords());
  core::expect::sizeMatches(src, dst);
  for (const auto &item : src) {
    const auto &dst_ = dst[item.name()];
    core::expect::sizeMatches(item.attrs(), dst_.attrs());
    core::expect::sizeMatches(item.masks(), dst_.masks());
    copy_or_match(item.data(), dst_.data(), dim, srcIndices, dstIndices);
    for (const auto &[name, var] : item.masks())
      copy_or_match(var, dst_.masks()[name], dim, srcIndices, dstIndices);
    for (const auto &[name, var] : item.attrs())
      copy_or_match(var, dst_.attrs()[name], dim, srcIndices, dstIndices);
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
                                                  var.hasVariances())
             : Variable(var);
};
}

// TODO These functions are an unfortunate near-duplicate of `resize`. However,
// the latter drops coords along the resized dimension. Is there a way to unify
// this? Can the need to drop coords in resize be avoided?
DataArray resize_default_init(const DataArrayConstView &parent, const Dim dim,
                              const scipp::index size) {
  DataArray buffer(copy_or_resize(parent.data(), dim, size));
  for (const auto &[name, var] : parent.coords())
    buffer.coords().set(name, copy_or_resize(var, dim, size));
  for (const auto &[name, var] : parent.masks())
    buffer.masks().set(name, copy_or_resize(var, dim, size));
  for (const auto &[name, var] : parent.attrs())
    buffer.attrs().set(name, copy_or_resize(var, dim, size));
  return buffer;
}

Dataset resize_default_init(const DatasetConstView &parent, const Dim dim,
                            const scipp::index size) {
  Dataset buffer;
  for (const auto &[name, var] : parent.coords())
    buffer.coords().set(name, copy_or_resize(var, dim, size));
  for (const auto &item : parent) {
    buffer.setData(item.name(), copy_or_resize(item.data(), dim, size));
    for (const auto &[name, var] : item.masks())
      buffer[item.name()].masks().set(name, copy_or_resize(var, dim, size));
    for (const auto &[name, var] : item.attrs())
      buffer[item.name()].attrs().set(name, copy_or_resize(var, dim, size));
  }
  return buffer;
}

template <class T>
Variable make_bins_impl(Variable &&indices, const Dim dim, T &&buffer) {
  return {std::make_unique<variable::DataModel<bucket<T>>>(
      std::move(indices), dim, std::move(buffer))};
}

/// Construct a bin-variable over a data array.
///
/// Each bin is represented by a VariableView. `indices` defines the array of
/// bins as slices of `buffer` along `dim`.
Variable make_bins(Variable indices, const Dim dim, DataArray buffer) {
  return make_bins_impl(std::move(indices), dim, std::move(buffer));
}

/// Construct a bin-variable over a dataset.
///
/// Each bin is represented by a VariableView. `indices` defines the array of
/// bins as slices of `buffer` along `dim`.
Variable make_bins(Variable indices, const Dim dim, Dataset buffer) {
  return make_bins_impl(std::move(indices), dim, std::move(buffer));
}

Variable make_non_owning_bins(const VariableView &indices, const Dim dim,
                              const DataArrayView &buffer) {
  return {std::make_unique<variable::DataModel<bucket<DataArrayView>>>(
      indices, dim, buffer)};
}

Variable make_non_owning_bins(const VariableConstView &indices, const Dim dim,
                              const DataArrayConstView &buffer) {
  return {std::make_unique<variable::DataModel<bucket<DataArrayConstView>>>(
      indices, dim, buffer)};
}

namespace {
template <class T> Variable bucket_sizes_impl(const VariableConstView &view) {
  const auto &indices = std::get<0>(view.constituents<bucket<T>>());
  const auto [begin, end] = unzip(indices);
  return end - begin;
}
} // namespace

Variable bucket_sizes(const VariableConstView &var) {
  if (var.dtype() == dtype<bucket<Variable>>)
    return bucket_sizes_impl<Variable>(var);
  else if (var.dtype() == dtype<bucket<DataArray>>)
    return bucket_sizes_impl<DataArray>(var);
  else if (var.dtype() == dtype<bucket<Dataset>>)
    return bucket_sizes_impl<Dataset>(var);
  else
    return makeVariable<scipp::index>(var.dims());
}

DataArray bucket_sizes(const DataArrayConstView &array) {
  return {bucket_sizes(array.data()), array.coords(), array.masks(),
          array.attrs()};
}

Dataset bucket_sizes(const DatasetConstView &dataset) {
  return apply_to_items(dataset, [](auto &&_) { return bucket_sizes(_); });
}

bool is_buckets(const DataArrayConstView &array) {
  return is_buckets(array.data());
}

bool is_buckets(const DatasetConstView &dataset) {
  return std::any_of(dataset.begin(), dataset.end(),
                     [](const auto &item) { return is_buckets(item); });
}

} // namespace scipp::dataset

namespace scipp::dataset::buckets {
namespace {

template <class T>
auto combine(const VariableConstView &var0, const VariableConstView &var1) {
  const auto &[indices0, dim0, buffer0] = var0.constituents<bucket<T>>();
  const auto &[indices1, dim1, buffer1] = var1.constituents<bucket<T>>();
  const Dim dim = dim0;
  const auto [begin0, end0] = unzip(indices0);
  const auto [begin1, end1] = unzip(indices1);
  const auto sizes0 = end0 - begin0;
  const auto sizes1 = end1 - begin1;
  const auto sizes = sizes0 + sizes1;
  const auto end = cumsum(sizes);
  const auto begin = end - sizes;
  auto buffer = resize_default_init(
      buffer0, dim, end.template values<scipp::index>().as_span().back());
  copy_slices(buffer0, buffer, dim, indices0, zip(begin, end - sizes1));
  copy_slices(buffer1, buffer, dim, indices1, zip(begin + sizes0, end));
  return variable::DataModel<bucket<T>>{zip(begin, end), dim,
                                        std::move(buffer)};
}

template <class T>
auto concatenate_impl(const VariableConstView &var0,
                      const VariableConstView &var1) {
  return Variable{
      std::make_unique<variable::DataModel<bucket<T>>>(combine<T>(var0, var1))};
}

template <class T>
void reserve_impl(const VariableView &var, const VariableConstView &shape) {
  // TODO this only reserves in the bins, but assumes buffer has enough space
  const auto &[indices, dim, buffer] = var.constituents<bucket<T>>();
  variable::transform_in_place(
      indices, shape,
      overloaded{
          core::element::arg_list<std::tuple<scipp::index_pair, scipp::index>>,
          core::keep_unit,
          [](auto &begin_end, auto &size) { begin_end.second += size; }});
}

} // namespace

void reserve(const VariableView &var, const VariableConstView &shape) {
  if (var.dtype() == dtype<bucket<Variable>>)
    return reserve_impl<Variable>(var, shape);
  else if (var.dtype() == dtype<bucket<DataArray>>)
    return reserve_impl<DataArray>(var, shape);
  else
    return reserve_impl<Dataset>(var, shape);
}

Variable concatenate(const VariableConstView &var0,
                     const VariableConstView &var1) {
  if (var0.dtype() == dtype<bucket<Variable>>)
    return concatenate_impl<Variable>(var0, var1);
  else if (var0.dtype() == dtype<bucket<DataArray>>)
    return concatenate_impl<DataArray>(var0, var1);
  else
    return concatenate_impl<Dataset>(var0, var1);
}

DataArray concatenate(const DataArrayConstView &a,
                      const DataArrayConstView &b) {
  return {buckets::concatenate(a.data(), b.data()),
          union_(a.coords(), b.coords()), union_or(a.masks(), b.masks()),
          intersection(a.attrs(), b.attrs())};
}

/// Reduce a dimension by concatenating all elements along the dimension.
///
/// This is the analogue to summing non-bucket data.
Variable concatenate(const VariableConstView &var, const Dim dim) {
  if (var.dtype() == dtype<bucket<Variable>>)
    return concat_bins<Variable>(var, dim);
  else
    return concat_bins<DataArray>(var, dim);
}

/// Reduce a dimension by concatenating all elements along the dimension.
///
/// This is the analogue to summing non-bucket data.
DataArray concatenate(const DataArrayConstView &array, const Dim dim) {
  return bin(array, {}, {}, {dim});
}

void append(const VariableView &var0, const VariableConstView &var1) {
  if (var0.dtype() == dtype<bucket<Variable>>)
    var0.replace_model(combine<Variable>(var0, var1));
  else if (var0.dtype() == dtype<bucket<DataArray>>)
    var0.replace_model(combine<DataArray>(var0, var1));
  else
    var0.replace_model(combine<Dataset>(var0, var1));
}

void append(const DataArrayView &a, const DataArrayConstView &b) {
  expect::coordsAreSuperset(a, b);
  union_or_in_place(a.masks(), b.masks());
  append(a.data(), b.data());
}

Variable histogram(const VariableConstView &data,
                   const VariableConstView &binEdges) {
  using namespace scipp::core;
  auto hist_dim = binEdges.dims().inner();
  const auto &[indices, dim, buffer] = data.constituents<bucket<DataArray>>();
  const Masker masker(buffer, dim);
  return variable::transform_subspan(
      buffer.dtype(), hist_dim, binEdges.dims()[hist_dim] - 1,
      subspan_view(buffer.meta()[hist_dim], dim, indices),
      subspan_view(masker.data(), dim, indices), binEdges, element::histogram);
}

Variable map(const DataArrayConstView &function, const VariableConstView &x,
             Dim dim) {
  if (dim == Dim::Invalid)
    dim = edge_dimension(function);
  const Masker masker(function, dim);
  const auto &coord = bins_view<DataArray>(x).meta()[dim];
  const auto &edges = function.meta()[dim];
  const auto weights = subspan_view(masker.data(), dim);
  if (all(is_linspace(edges, dim)).value<bool>()) {
    return variable::transform(coord, subspan_view(edges, dim), weights,
                               core::element::event::map_linspace);
  } else {
    if (!is_sorted(edges, dim))
      throw except::BinEdgeError("Bin edges of histogram must be sorted.");
    return variable::transform(coord, subspan_view(edges, dim), weights,
                               core::element::event::map_sorted_edges);
  }
}

void scale(const DataArrayView &array, const DataArrayConstView &histogram,
           Dim dim) {
  if (dim == Dim::Invalid)
    dim = edge_dimension(histogram);
  // Coords along dim are ignored since "binning" is dynamic for buckets.
  expect::coordsAreSuperset(array, histogram.slice({dim, 0}));
  // scale applies masks along dim but others are kept
  union_or_in_place(array.masks(), histogram.slice({dim, 0}).masks());
  const Masker masker(histogram, dim);
  auto data = bins_view<DataArray>(array.data()).data();
  const auto &coord = bins_view<DataArray>(array.data()).meta()[dim];
  const auto &edges = histogram.meta()[dim];
  const auto weights = subspan_view(masker.data(), dim);
  if (all(is_linspace(edges, dim)).value<bool>()) {
    transform_in_place(data, coord, subspan_view(edges, dim), weights,
                       core::element::event::map_and_mul_linspace);
  } else {
    if (!is_sorted(edges, dim))
      throw except::BinEdgeError("Bin edges of histogram must be sorted.");
    transform_in_place(data, coord, subspan_view(edges, dim), weights,
                       core::element::event::map_and_mul_sorted_edges);
  }
}

Variable sum(const VariableConstView &data) {
  auto type = variable::variableFactory().elem_dtype(data);
  type = type == dtype<bool> ? dtype<int64_t> : type;
  const auto unit = variable::variableFactory().elem_unit(data);
  Variable summed;
  if (variable::variableFactory().hasVariances(data))
    summed = Variable(type, data.dims(), unit, Values{}, Variances{});
  else
    summed = Variable(type, data.dims(), unit, Values{});
  variable::sum_impl(summed, data);
  return summed;
}

DataArray sum(const DataArrayConstView &data) {
  return {buckets::sum(data.data()), data.coords(), data.masks(), data.attrs()};
}

Dataset sum(const DatasetConstView &d) {
  return apply_to_items(d, [](auto &&... _) { return buckets::sum(_...); });
}

} // namespace scipp::dataset::buckets
