// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <string>

#include "scipp/core/eigen.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

INSTANTIATE_STRUCTURE_VARIABLE(vector_3_float64, Eigen::Vector3d)
INSTANTIATE_STRUCTURE_VARIABLE(matrix_3_float64, Eigen::Matrix3d)

template SCIPP_VARIABLE_EXPORT Variable
Variable::elements<Eigen::Vector3d, scipp::index>(const scipp::index &) const;

template SCIPP_VARIABLE_EXPORT Variable
Variable::elements<Eigen::Matrix3d, scipp::index, scipp::index>(
    const scipp::index &, const scipp::index &) const;

} // namespace scipp::variable
