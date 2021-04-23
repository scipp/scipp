// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dimensions.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/except.h"
#include "scipp/variable/shape.h"
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

Variable broadcast(const Variable &var, const Dimensions &dims) {
  auto result = variableFactory().empty_like(var, dims);
  result.data().copy(var, result);
  return result;
}

Variable concatenate(const Variable &a1, const Variable &a2, const Dim dim) {
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
    out = Variable(a1, dims);
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

Variable resize(const Variable &var, const Dim dim, const scipp::index size) {
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
Variable resize(const Variable &var, const Variable &shape) {
  return {shape.dims(), var.data().makeDefaultFromParent(shape)};
}

namespace {
void swap(Variable &var, const Dim dim, const scipp::index a,
          const scipp::index b) {
  const Variable tmp = copy(var.slice({dim, a}));
  copy(var.slice({dim, b}), var.slice({dim, a}));
  copy(tmp, var.slice({dim, b}));
}
} // namespace

Variable reverse(const Variable &var, const Dim dim) {
  auto out = copy(var);
  const auto size = out.dims()[dim];
  for (scipp::index i = 0; i < size / 2; ++i)
    swap(out, dim, i, size - i - 1);
  return out;
}

Variable reshape(const Variable &var, const Dimensions &dims) {
  expect_same_volume(var.dims(), dims);
  // We could be less restrictive here, avoiding copies whenever we are
  // reshaping an ordered contiguous chunk.
  Variable reshaped =
      var.dims() == var.array_params().dataDims() ? var : copy(var);
  reshaped.setDims(dims);
  return reshaped;
}

Variable fold(const Variable &view, const Dim from_dim,
              const Dimensions &to_dims) {
  return reshape(view, fold(view.dims(), from_dim, to_dims));
}

Variable flatten(const Variable &view,
                 const scipp::span<const Dim> &from_labels, const Dim to_dim) {
  return reshape(view, flatten(view.dims(), from_labels, to_dim));
}

Variable transpose(const Variable &var, const std::vector<Dim> &dims) {
  return var.transpose(dims);
}

void squeeze(Variable &var, const std::vector<Dim> &dims) {
  auto squeezed = var.dims();
  for (const auto &dim : dims) {
    if (squeezed[dim] != 1)
      throw except::DimensionError("Cannot squeeze '" + to_string(dim) +
                                   "' since it is not of length 1.");
    squeezed.erase(dim);
  }
  var.setDims(squeezed);
}

} // namespace scipp::variable
