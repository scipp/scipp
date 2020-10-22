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
#include "scipp/dataset/bucket.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/buckets.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/transform_subspan.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "../variable/operations_common.h"
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
  core::expect::sizeMatches(src.coords(), dst.coords());
  core::expect::sizeMatches(src.masks(), dst.masks());
  for (const auto &[name, coord] : src.coords())
    copy_or_match(coord, dst.coords()[name], dim, srcIndices, dstIndices);
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
    core::expect::sizeMatches(item.unaligned_coords(), dst_.unaligned_coords());
    core::expect::sizeMatches(item.masks(), dst_.masks());
    copy_or_match(item.data(), dst_.data(), dim, srcIndices, dstIndices);
    for (const auto &[name, var] : item.masks())
      copy_or_match(var, dst_.masks()[name], dim, srcIndices, dstIndices);
    for (const auto &[name, var] : item.unaligned_coords())
      copy_or_match(var, dst_.coords()[name], dim, srcIndices, dstIndices);
  }
}

} // namespace scipp::dataset

namespace scipp::dataset::buckets {

std::tuple<Variable, scipp::index>
sizes_to_begin(const VariableConstView &sizes) {
  Variable begin(sizes);
  scipp::index size = 0;
  for (auto &i : begin.values<scipp::index>()) {
    const auto old_size = size;
    size += i;
    i = old_size;
  }
  return {begin, size};
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

auto resize_buffer(const VariableConstView &parent, const Dim dim,
                   const scipp::index size) {
  return copy_or_resize(parent, dim, size);
}

// TODO These functions are an unfortunate near-duplicate of `resize`. However,
// the latter drops coords along the resized dimension. Is there a way to unify
// this? Can the need to drop coords in resize be avoided?
auto resize_buffer(const DataArrayConstView &parent, const Dim dim,
                   const scipp::index size) {
  DataArray buffer(copy_or_resize(parent.data(), dim, size));
  for (const auto &[name, var] : parent.aligned_coords())
    buffer.aligned_coords().set(name, copy_or_resize(var, dim, size));
  for (const auto &[name, var] : parent.masks())
    buffer.masks().set(name, copy_or_resize(var, dim, size));
  for (const auto &[name, var] : parent.unaligned_coords())
    buffer.unaligned_coords().set(name, copy_or_resize(var, dim, size));
  return buffer;
}

auto resize_buffer(const DatasetConstView &parent, const Dim dim,
                   const scipp::index size) {
  Dataset buffer;
  for (const auto &[name, var] : parent.coords())
    buffer.coords().set(name, copy_or_resize(var, dim, size));
  for (const auto &item : parent) {
    buffer.setData(item.name(), copy_or_resize(item.data(), dim, size));
    for (const auto &[name, var] : item.masks())
      buffer[item.name()].masks().set(name, copy_or_resize(var, dim, size));
    for (const auto &[name, var] : item.unaligned_coords())
      buffer[item.name()].coords().set(name, copy_or_resize(var, dim, size));
  }
  return buffer;
}

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
  const auto [begin, size] = sizes_to_begin(sizes);
  const auto end = begin + sizes;
  auto buffer = resize_buffer(buffer0, dim, size);
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
} // namespace

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
          union_(a.aligned_coords(), b.aligned_coords()),
          union_or(a.masks(), b.masks()),
          intersection(a.unaligned_coords(), b.unaligned_coords())};
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

namespace histogram_detail {
template <class Out, class Coord, class Weight, class Edge>
using args = std::tuple<span<Out>, span<const Coord>, span<const Weight>,
                        span<const Edge>>;
}

Variable histogram(const VariableConstView &data,
                   const VariableConstView &binEdges) {
  using namespace scipp::core;
  using namespace histogram_detail;
  auto hist_dim = binEdges.dims().inner();
  const auto &[indices, dim, buffer] = data.constituents<bucket<DataArray>>();
  if (!buffer.masks().empty())
    throw std::runtime_error("Masked data cannot be histogrammed yet.");
  VariableConstView spans(indices);
  Variable merged;
  if (indices.dims().contains(hist_dim)) {
    const auto size = indices.dims()[hist_dim];
    const auto [begin, end] = unzip(indices);
    // Only contiguous ranges along histogramming dim supported at this point.
    core::expect::equals(begin.slice({hist_dim, 1, size}),
                         end.slice({hist_dim, 0, size - 1}));
    merged = zip(begin.slice({hist_dim, 0}), end.slice({hist_dim, size - 1}));
    spans = VariableConstView(merged);
  }
  return variable::transform_subspan<std::tuple<
      args<double, double, double, double>, args<double, float, double, double>,
      args<double, float, double, float>, args<double, double, float, double>>>(
      buffer.dtype(), hist_dim, binEdges.dims()[hist_dim] - 1,
      subspan_view(buffer.coords()[hist_dim], dim, spans),
      subspan_view(buffer.data(), dim, spans), binEdges, element::histogram);
}

Variable map(const DataArrayConstView &function, const VariableConstView &x,
             Dim hist_dim) {
  if (hist_dim == Dim::Invalid)
    hist_dim = edge_dimension(function);
  const auto mask = irreducible_mask(function.masks(), hist_dim);
  Variable masked;
  if (mask)
    masked = function.data() * ~mask;
  const auto &[indices, dim, buffer] = x.constituents<bucket<DataArray>>();
  // Note the current inefficiency here: Output buffer is created with full
  // size, even if `x` is a slice and only subsections of the buffer are needed.
  auto out = variable::variableFactory().create(
      function.dtype(), buffer.dims(), units::one, function.hasVariances());
  // TODO "bug" here: subspan_view creates a new variable, so out unit not set!
  transform_in_place(subspan_view(out, dim, indices),
                     subspan_view(buffer.coords()[hist_dim], dim, indices),
                     subspan_view(function.coords()[hist_dim], hist_dim),
                     subspan_view(mask ? masked : function.data(), hist_dim),
                     core::element::event::map_in_place);
  // TODO Workaround, see comment above
  out.setUnit(function.unit());
  return Variable{std::make_unique<variable::DataModel<bucket<Variable>>>(
      indices, dim, std::move(out))};
}

void scale(const DataArrayView &data, const DataArrayConstView &histogram) {
  const auto dim = edge_dimension(histogram);
  // Coords along dim are ignored since "binning" is dynamic for buckets.
  expect::coordsAreSuperset(data, histogram.slice({dim, 0}));
  // buckets::map applies masks along dim
  union_or_in_place(data.masks(), histogram.slice({dim, 0}).masks());
  // The result of buckets::map is a variable, i.e., we cannot rely on the
  // multiplication taking care of mask propagation and coord checks, hence the
  // handling above.
  data *= map(histogram, data.data(), histogram.dims().inner());
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
  return {buckets::sum(data.data()), data.aligned_coords(), data.masks(),
          data.unaligned_coords()};
}

template <class T>
Variable from_constituents_impl(Variable &&indices, const Dim dim, T &&buffer) {
  return {std::make_unique<variable::DataModel<bucket<T>>>(
      std::move(indices), dim, std::move(buffer))};
}

Variable from_constituents(Variable &&indices, const Dim dim,
                           Variable &&buffer) {
  return from_constituents_impl(std::move(indices), dim, std::move(buffer));
}

Variable from_constituents(Variable &&indices, const Dim dim,
                           DataArray &&buffer) {
  return from_constituents_impl(std::move(indices), dim, std::move(buffer));
}

Variable from_constituents(Variable &&indices, const Dim dim,
                           Dataset &&buffer) {
  return from_constituents_impl(std::move(indices), dim, std::move(buffer));
}

} // namespace scipp::dataset::buckets
