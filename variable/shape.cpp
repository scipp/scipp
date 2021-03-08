// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/shape.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/except.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

using namespace scipp::core;

namespace scipp::variable {

void expect_same_volume(const Dimensions &old_dims,
                        const Dimensions &new_dims) {
  if (old_dims.volume() != new_dims.volume())
    throw except::DimensionError(
        "Cannot reshape to dimensions with different volume");
}

Variable broadcast(const VariableConstView &var, const Dimensions &dims) {
  auto result = variableFactory().empty_like(var, dims);
  result.data().copy(var, result);
  return result;
}

Variable concatenate(const VariableConstView &a1, const VariableConstView &a2,
                     const Dim dim) {
  if (a1.dtype() != a2.dtype())
    throw std::runtime_error(
        "Cannot concatenate Variables: Data types do not match.");
  if (a1.unit() != a2.unit())
    throw std::runtime_error(
        "Cannot concatenate Variables: Units do not match.");

  const auto &dims1 = a1.dims();
  const auto &dims2 = a2.dims();
  // TODO Many things in this function should be refactored and moved in class
  // Dimensions.
  // TODO Special handling for edge variables.
  for (const auto &dim1 : dims1.labels()) {
    if (dim1 != dim) {
      if (!dims2.contains(dim1))
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimensions do not match.");
      if (dims2[dim1] != dims1[dim1])
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimension extents do not match.");
    }
  }
  auto size1 = dims1.shape().size();
  auto size2 = dims2.shape().size();
  if (dims1.contains(dim))
    size1--;
  if (dims2.contains(dim))
    size2--;
  // This check covers the case of dims2 having extra dimensions not present in
  // dims1.
  // TODO Support broadcast of dimensions?
  if (size1 != size2)
    throw std::runtime_error(
        "Cannot concatenate Variables: Dimensions do not match.");

  Variable out;
  auto dims(dims1);
  scipp::index extent1 = 1;
  scipp::index extent2 = 1;
  if (dims1.contains(dim))
    extent1 += dims1[dim] - 1;
  if (dims2.contains(dim))
    extent2 += dims2[dim] - 1;
  if (dims.contains(dim))
    dims.resize(dim, extent1 + extent2);
  else
    dims.add(dim, extent1 + extent2);
  if (is_bins(a1)) {
    constexpr auto bin_sizes = [](const auto &ranges) {
      const auto [begin, end] = unzip(ranges);
      return end - begin;
    };
    out = empty_like(a1, {},
                     concatenate(bin_sizes(a1.bin_indices()),
                                 bin_sizes(a2.bin_indices()), dim));
  } else {
    out = Variable(a1);
    out.setDims(dims);
  }

  out.data().copy(a1, out.slice({dim, 0, extent1}));
  out.data().copy(a2, out.slice({dim, extent1, extent1 + extent2}));

  return out;
}

Variable permute(const Variable &var, const Dim dim,
                 const std::vector<scipp::index> &indices) {
  auto permuted(var);
  for (scipp::index i = 0; i < scipp::size(indices); ++i)
    permuted.data().copy(var.slice({dim, i}),
                         permuted.slice({dim, indices[i]}));
  return permuted;
}

Variable resize(const VariableConstView &var, const Dim dim,
                const scipp::index size) {
  auto dims = var.dims();
  dims.resize(dim, size);
  return Variable(var, dims);
}

/// Return new variable resized to given shape.
///
/// For bucket variables the values of `shape` are interpreted as bucket sizes
/// to RESERVE and the buffer is also resized accordingly. The emphasis is on
/// "reserve", i.e., buffer size and begin indices are set up accordingly, but
/// end=begin is set, i.e., the buckets are empty, but may be grown up to the
/// requested size. For normal (non-bucket) variable the values of `shape` are
/// ignored, i.e., only `shape.dims()` is used to determine the shape of the
/// output.
Variable resize(const VariableConstView &var, const VariableConstView &shape) {
  return Variable(var, var.underlying().data().makeDefaultFromParent(shape));
}

namespace {
void swap(Variable &var, const Dim dim, const scipp::index a,
          const scipp::index b) {
  const Variable tmp(var.slice({dim, a}));
  var.slice({dim, a}).assign(var.slice({dim, b}));
  var.slice({dim, b}).assign(tmp);
}
} // namespace

Variable reverse(Variable var, const Dim dim) {
  const auto size = var.dims()[dim];
  for (scipp::index i = 0; i < size / 2; ++i)
    swap(var, dim, i, size - i - 1);
  return var;
}

VariableView reshape(Variable &var, const Dimensions &dims) {
  return {var, dims};
}

Variable reshape(Variable &&var, const Dimensions &dims) {
  expect_same_volume(var.dims(), dims);
  Variable reshaped(std::move(var));
  reshaped.setDims(dims);
  return reshaped;
}

Variable reshape(const VariableConstView &view, const Dimensions &dims) {
  // In general a variable slice is not contiguous. Therefore we cannot reshape
  // without making a copy (except for special cases).
  expect_same_volume(view.dims(), dims);
  Variable reshaped(view);
  reshaped.setDims(dims);
  return reshaped;
}

void validate_split_dims(const Dimensions &old_dims, const Dim from_dim,
                         const Dimensions &to_dims) {
  if (!old_dims.contains(from_dim))
    throw except::DimensionError("Dimension to split not found.");
  // Make sure that new dims do not already exist in data dimensions,
  // apart from the old dim (i.e. old dim can be re-used)
  for (const auto dim : to_dims.labels())
    if (old_dims.contains(dim) && dim != from_dim)
      throw except::DimensionError(
          "Split: new dimensions cannot contain labels that already exist.");
}

void validate_flatten_dims(const Dimensions &old_dims,
                           const Dimensions &from_dims, const Dim to_dim) {
  for (const auto dim : from_dims.labels())
    if (!old_dims.contains(dim))
      throw except::DimensionError(
          "Flatten: dimension to be flattened not found.");
  // Make sure that new dim does not already exist in data dimensions,
  // apart from one of the from_dims (i.e. an old dim can be re-used)
  if (old_dims.contains(to_dim) && !from_dims.contains(to_dim))
    throw except::DimensionError(
        "Flatten: final flattened dimension already exists.");
  // Only allow reshaping contiguous dimensions.
  // Note that isContiguousIn only allows for inner contiguous blocks,
  // and contains(dimensions) ignores dimension order.
  const auto offset = old_dims.index(from_dims.labels()[0]);
  for (scipp::index i = 0; i < from_dims.ndim(); ++i) {
    if (old_dims.label(i + offset) != from_dims.label(i) ||
        old_dims.size(i + offset) != from_dims.size(i))
      throw except::DimensionError(
          "Flatten: can only flatten a contiguous set of dimensions. "
          "The order of the dimensions to flatten must also match the order of "
          "dimensions in the original Variable or DataArray.");
  }
}

/// Split dims for reshaping one dim into multiple dims
///
/// Go through the old dims and:
/// - if the dim does not equal the dim that is being stacked, copy dim/shape
/// - if the dim equals the dim to be stacked, replace by stack of new dims
Dimensions split_dims(const Dimensions &old_dims, const Dim from_dim,
                      const Dimensions &to_dims) {
  Dimensions new_dims;
  for (const auto dim : old_dims.labels())
    if (dim != from_dim)
      new_dims.addInner(dim, old_dims[dim]);
    else
      for (const auto lab : to_dims.labels())
        new_dims.addInner(lab, to_dims[lab]);
  return new_dims;
}

/// Flatten dims for reshaping multiple dims into one
///
/// Go through the old dims and:
/// - if the dim is contained in the list of dims to be flattened, add the new
///   dim once
/// - if not, copy the dim/shape
Dimensions flatten_dims(const Dimensions &old_dims, const Dimensions &from_dims,
                        const Dim to_dim) {
  Dimensions new_dims;
  for (const auto dim : old_dims.labels())
    if (from_dims.contains(dim)) {
      if (!new_dims.contains(to_dim))
        new_dims.addInner(to_dim, from_dims.volume());
    } else {
      new_dims.addInner(dim, old_dims[dim]);
    }
  return new_dims;
}

Variable split(const VariableConstView &view, const Dim from_dim,
               const Dimensions &to_dims) {
  validate_split_dims(view.dims(), from_dim, to_dims);
  return reshape(view, split_dims(view.dims(), from_dim, to_dims));
}

Variable flatten(const VariableConstView &view,
                 const std::vector<Dim> &from_labels, const Dim to_dim) {
  const auto &view_dims = view.dims();
  Dimensions from_dims;
  for (const auto dim : from_labels)
    from_dims.addInner(dim, view_dims[dim]);
  return reshape(view, flatten_dims(view_dims, from_dims, to_dim));
}

VariableView transpose(Variable &var, const std::vector<Dim> &dims) {
  return transpose(VariableView(var), dims);
}

Variable transpose(Variable &&var, const std::vector<Dim> &dims) {
  return Variable(transpose(VariableConstView(var), dims));
}

VariableConstView transpose(const VariableConstView &view,
                            const std::vector<Dim> &dims) {
  return view.transpose(dims);
}

VariableView transpose(const VariableView &view, const std::vector<Dim> &dims) {
  return view.transpose(dims);
}

void squeeze(Variable &var, const std::vector<Dim> &dims) {
  auto squeezed = var.dims();
  for (const auto dim : dims) {
    if (squeezed[dim] != 1)
      throw except::DimensionError("Cannot squeeze '" + to_string(dim) +
                                   "' since it is not of length 1.");
    squeezed.erase(dim);
  }
  var.setDims(squeezed);
}

} // namespace scipp::variable
