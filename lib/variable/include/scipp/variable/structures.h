// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

template <class T, class Elem>
[[nodiscard]] Variable make_structures(const Dimensions &dims,
                                       const sc_units::Unit &unit,
                                       element_array<double> &&values);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
make_vectors(const Dimensions &dims, const sc_units::Unit &unit,
             element_array<double> &&values);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
make_matrices(const Dimensions &dims, const sc_units::Unit &unit,
              element_array<double> &&values);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
make_affine_transforms(const Dimensions &dims, const sc_units::Unit &unit,
                       element_array<double> &&values);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
make_rotations(const Dimensions &dims, const sc_units::Unit &unit,
               element_array<double> &&values);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
make_translations(const Dimensions &dims, const sc_units::Unit &unit,
                  element_array<double> &&values);

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::vector<std::string>
element_keys(const Variable &var);

} // namespace scipp::variable
