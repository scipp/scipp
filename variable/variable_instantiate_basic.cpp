// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <string>

#include "scipp/variable/variable.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

INSTANTIATE_VARIABLE(string, std::string)
INSTANTIATE_VARIABLE(float64, double)
INSTANTIATE_VARIABLE(float32, float)
INSTANTIATE_VARIABLE(int64, int64_t)
INSTANTIATE_VARIABLE(int32, int32_t)
INSTANTIATE_VARIABLE(bool, bool)
INSTANTIATE_VARIABLE(vector_3_float64, Eigen::Vector3d)
INSTANTIATE_VARIABLE(matrix_3_float64, Eigen::Matrix3d)
INSTANTIATE_VARIABLE(event_list_float64, event_list<double>)
INSTANTIATE_VARIABLE(event_list_float32, event_list<float>)
INSTANTIATE_VARIABLE(event_list_int64, event_list<int64_t>)
INSTANTIATE_VARIABLE(event_list_int32, event_list<int32_t>)
INSTANTIATE_VARIABLE(event_list_bool, event_list<bool>)

} // namespace scipp::variable
