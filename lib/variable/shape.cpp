// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/core/dimensions.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
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

namespace {
auto get_bin_sizes(const std::span<const Variable> vars) {
  std::vector<Variable> sizes;
  sizes.reserve(vars.size());
  for (const auto &var : vars)
    sizes.emplace_back(bin_sizes(var));
  return sizes;
}
} // namespace

Variable concat(const std::span<const Variable> vars, const Dim dim) {
  if (vars.empty())
    throw std::invalid_argument("Cannot concat empty list.");
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
  Variable out;
  if (is_bins(vars.front())) {
    out = empty_like(vars.front(), {}, concat(get_bin_sizes(vars), dim));
  } else {
    out = empty_like(vars.front(), dims);
  }
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

Variable flatten(const Variable &view, const std::span<const Dim> &from_labels,
                 const Dim to_dim) {
  if (from_labels.empty()) {
    auto out(view);
    out.unchecked_dims().addInner(to_dim, 1);
    out.unchecked_strides().push_back(1);
    return out;
  }
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

Variable transpose(const Variable &var, const std::span<const Dim> dims) {
  return var.transpose(dims);
}

std::vector<scipp::Dim>
dims_for_squeezing(const core::Sizes &data_dims,
                   const std::optional<std::span<const Dim>> selected_dims) {
  if (selected_dims.has_value()) {
    for (const auto &dim : *selected_dims) {
      if (const auto size = data_dims[dim]; size != 1)
        throw except::DimensionError("Cannot squeeze '" + to_string(dim) +
                                     "' of length " + std::to_string(size) +
                                     ", must be of length 1.");
    }
    return std::vector<Dim>{selected_dims->begin(), selected_dims->end()};
  } else {
    std::vector<Dim> length_1_dims;
    length_1_dims.reserve(data_dims.size());
    for (const auto &dim : data_dims) {
      if (data_dims[dim] == 1) {
        length_1_dims.push_back(dim);
      }
    }
    return length_1_dims;
  }
}

Variable squeeze(const Variable &var,
                 const std::optional<std::span<const Dim>> dims) {
  auto squeezed = var;
  for (const auto &dim : dims_for_squeezing(var.dims(), dims)) {
    squeezed = squeezed.slice({dim, 0});
  }
  return squeezed;
}

} // namespace scipp::variable
