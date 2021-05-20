// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/geometric_operations.h"
#include "scipp/core/element/special_values.h"
#include "scipp/core/element/util.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_concept.h"

#include "operations_common.h"

using namespace scipp::core;

namespace scipp::variable {

/// Return a deep copy of a Variable.
Variable copy(const Variable &var) {
  Variable out(empty_like(var));
  out.data().copy(var, out);
  return out;
}

/// Copy variable to output variable.
Variable &copy(const Variable &var, Variable &out) {
  var.data().copy(var, out);
  return out;
}

/// Copy variable to output variable.
Variable copy(const Variable &var, Variable &&out) {
  copy(var, out);
  return std::move(out);
}

Variable masked_to_zero(const Variable &var, const Variable &mask) {
  return variable::transform(var, mask, element::convertMaskedToZero,
                             "masked_to_zero");
}

namespace geometry {
Variable position(const Variable &x, const Variable &y, const Variable &z) {
  return transform(x, y, z, element::geometry::position, "linalg.position");
}
} // namespace geometry

} // namespace scipp::variable
