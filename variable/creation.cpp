// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/creation.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

/// Create empty (uninitialized) variable with same parameters as prototype.
///
/// If specified, `shape` defines the shape and dims of the output. If
/// `prototype` contains binned data the values of `shape` are interpreted as
/// bin sizes.
Variable empty_like(const VariableConstView &prototype,
                    const VariableConstView &shape) {
  return variableFactory().empty_like(prototype, shape);
}

} // namespace scipp::variable
