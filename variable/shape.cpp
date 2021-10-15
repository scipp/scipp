// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/core/dimensions.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/except.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_concept.h"
#include "scipp/variable/variable_factory.h"

using namespace scipp::core;

namespace scipp::variable {

Variable broadcast(const Variable &var, const Dimensions &dims) {
  return var.broadcast(dims);
}

Variable concatenate(const Variable &a1, const Variable &a2, const Dim dim) {
  if (a1.dtype() != a2.dtype())
    throw except::TypeError(
        "Cannot concatenate Variables: Data types do not match.");
  if (a1.unit() != a2.unit())
    throw except::UnitError(
        "Cannot concatenate Variables: Units do not match.");

  const auto &dims1 = a1.dims();
  const auto &dims2 = a2.dims();
  // TODO Many things in this function should be refactored and moved in class
  // Dimensions.
  for (const auto &dim1 : dims1.labels()) {
    if (dim1 != dim) {
      if (!dims2.contains(dim1))
        throw except::DimensionError(
            "Cannot concatenate Variables: Dimensions do not match.");
      if (dims2[dim1] != dims1[dim1])
        throw except::DimensionError(
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
    throw except::DimensionError(
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

Variable concat(const scipp::span<const Variable> vars, const Dim dim) {
  const auto it =
      std::find_if(vars.begin(), vars.end(),
                   [dim](const auto &var) { return var.dims().contains(dim); });
  Dimensions dims;
  // Expand dims for inputs that do not contain dim already. Favor order given
  // by first input, if not found add as outer dim.
  if (it == vars.end()) {
    dims = vars.front().dims();
    dims.add(dim, 1);
  } else {
    dims = it->dims();
    dims.resize(dim, 1);
  }
  std::vector<Variable> tmp;
  scipp::index size = 0;
  for (const auto &var : vars) {
    if (var.dims().contains(dim))
      tmp.emplace_back(var);
    else
      tmp.emplace_back(broadcast(var, dims));
    size += tmp.back().dims()[dim];
  }
  dims.resize(dim, size);
  auto out = empty_like(vars.front(), dims);
  scipp::index offset = 0;
  for (const auto &var : tmp) {
    const auto extent = var.dims()[dim];
    out.data().copy(var, out.slice({dim, offset, offset + extent}));
    offset += extent;
  }
  return out;
}

Variable resize(const Variable &var, const Dim dim, const scipp::index size,
                const FillValue fill) {
  auto dims = var.dims();
  dims.resize(dim, size);
  return special_like(broadcast(Variable(var, Dimensions{}), dims), fill);
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

Variable fold(const Variable &view, const Dim from_dim,
              const Dimensions &to_dims) {
  return view.fold(from_dim, to_dims);
}

Variable flatten(const Variable &view,
                 const scipp::span<const Dim> &from_labels, const Dim to_dim) {
  if (from_labels.empty())
    return broadcast(view, merge(view.dims(), Dimensions(to_dim, 1)));
  const auto &labels = view.dims().labels();
  auto it = std::search(labels.begin(), labels.end(), from_labels.begin(),
                        from_labels.end());
  if (it == labels.end())
    throw except::DimensionError("Can only flatten a contiguous set of "
                                 "dimensions in the correct order");
  scipp::index size = 1;
  auto to = std::distance(labels.begin(), it);
  auto out(view);
  for (const auto &from : from_labels) {
    size *= out.dims().size(to);
    if (from == from_labels.back()) {
      out.unchecked_dims().resize(from, size);
      out.unchecked_dims().replace_key(from, to_dim);
    } else {
      if (out.strides()[to] != out.dims().size(to + 1) * out.strides()[to + 1])
        return flatten(copy(view), from_labels, to_dim);
      out.unchecked_dims().erase(from);
      out.unchecked_strides().erase(to);
    }
  }
  return out;
}

Variable transpose(const Variable &var, const std::vector<Dim> &dims) {
  return var.transpose(dims);
}

Variable squeeze(const Variable &var, const std::vector<Dim> &dims) {
  auto squeezed = var;
  for (const auto &dim : dims) {
    if (squeezed.dims()[dim] != 1)
      throw except::DimensionError("Cannot squeeze '" + to_string(dim) +
                                   "' since it is not of length 1.");
    squeezed = squeezed.slice({dim, 0});
  }
  return squeezed;
}

} // namespace scipp::variable
