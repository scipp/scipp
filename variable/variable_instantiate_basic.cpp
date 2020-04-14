// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <string>

#include "scipp/variable/variable.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

INSTANTIATE_VARIABLE(std::string)
INSTANTIATE_VARIABLE(double)
INSTANTIATE_VARIABLE(float)
INSTANTIATE_VARIABLE(int64_t)
INSTANTIATE_VARIABLE(int32_t)
INSTANTIATE_VARIABLE(bool)
INSTANTIATE_VARIABLE(Eigen::Vector3d)
INSTANTIATE_VARIABLE(Eigen::Quaterniond)
INSTANTIATE_VARIABLE(sparse_container<double>)
INSTANTIATE_VARIABLE(sparse_container<float>)
INSTANTIATE_VARIABLE(sparse_container<int64_t>)
INSTANTIATE_VARIABLE(sparse_container<int32_t>)
INSTANTIATE_VARIABLE(sparse_container<bool>)

} // namespace scipp::variable
