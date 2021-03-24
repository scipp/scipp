// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/geometric_operations.h"
#include "scipp/core/element/special_values.h"
#include "scipp/core/element/util.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/transform.h"

#include "operations_common.h"

using namespace scipp::core;

namespace scipp::variable {

// Example of a "derived" operation: Implementation does not require adding a
// virtual function to VariableConcept.
std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<scipp::index> &indices) {
  if (indices.empty())
    return {var};
  std::vector<Variable> vars;
  vars.emplace_back(var.slice({dim, 0, indices.front()}));
  for (scipp::index i = 0; i < scipp::size(indices) - 1; ++i)
    vars.emplace_back(var.slice({dim, indices[i], indices[i + 1]}));
  vars.emplace_back(var.slice({dim, indices.back(), var.dims()[dim]}));
  return vars;
}

Variable filter(const Variable &var, const Variable &filter) {
  if (filter.dims().shape().size() != 1)
    throw std::runtime_error(
        "Cannot filter variable: The filter must by 1-dimensional.");
  const auto dim = filter.dims().labels()[0];
  auto mask = filter.values<bool>();

  const scipp::index removed = std::count(mask.begin(), mask.end(), false);
  if (removed == 0)
    return var;

  auto out(var);
  auto dims = out.dims();
  dims.resize(dim, dims[dim] - removed);
  out.setDims(dims);

  scipp::index iOut = 0;
  // Note: Could copy larger chunks of applicable for better(?) performance.
  // Note: This implementation is inefficient, since we need to cast to concrete
  // type for *every* slice. Should be combined into a single virtual call.
  for (scipp::index iIn = 0; iIn < scipp::size(mask); ++iIn)
    if (mask[iIn])
      out.data().copy(var.slice({dim, iIn}), out.slice({dim, iOut++}));
  return out;
}

/// Return a deep copy of a Variable.
Variable copy(const Variable &var) {
  Variable out(empty_like(var));
  out.data().copy(var, out);
  return out;
}

/// Copy variable to output variable.
Variable &copy(const Variable &var, Variable &out) {
  // TODO Is this missing dim/shape handling?
  var.data().copy(var, out);
  return out;
}

Variable masked_to_zero(const Variable &var, const Variable &mask) {
  return transform(var, mask, element::convertMaskedToZero);
}

namespace geometry {
Variable position(const Variable &x, const Variable &y, const Variable &z) {
  return transform(x, y, z, element::geometry::position);
}
Variable x(const Variable &pos) { return transform(pos, element::geometry::x); }
Variable y(const Variable &pos) { return transform(pos, element::geometry::y); }
Variable z(const Variable &pos) { return transform(pos, element::geometry::z); }
} // namespace geometry

} // namespace scipp::variable
