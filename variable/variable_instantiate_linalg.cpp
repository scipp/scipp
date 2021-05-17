// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

template <>
constexpr auto structure_element_offset<Eigen::Vector3d> =
    [](scipp::index i) { return i; };

template <>
constexpr auto structure_element_offset<Eigen::Matrix3d> =
    [](scipp::index i, scipp::index j) { return 3 * j + i; };

INSTANTIATE_STRUCTURE_VARIABLE(vector_3_float64, Eigen::Vector3d, double, 3,
                               scipp::index)
INSTANTIATE_STRUCTURE_VARIABLE(matrix_3_float64, Eigen::Matrix3d, double, 9,
                               scipp::index, scipp::index)

} // namespace scipp::variable
