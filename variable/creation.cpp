// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/creation.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

/// Create empty (uninitialized) variable with same parameters as prototype.
///
/// If specified, `shape` defines the shape of the output. If `prototype`
/// contains binned data `shape` may not be specified, instead `sizes` defines
/// the sizes of the desired bins.
Variable empty_like(const VariableConstView &prototype,
                    const std::optional<Dimensions> &shape,
                    const VariableConstView &sizes) {
  return variableFactory().empty_like(prototype, shape, sizes);
}

} // namespace scipp::variable
