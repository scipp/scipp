// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <string>

#include "scipp/core/variable.h"
#include "scipp/core/variable.tcc"

namespace scipp::core {

INSTANTIATE_VARIABLE(std::string)
INSTANTIATE_VARIABLE(double)
INSTANTIATE_VARIABLE(float)
INSTANTIATE_VARIABLE(int64_t)
INSTANTIATE_VARIABLE(int32_t)
INSTANTIATE_VARIABLE(bool)
INSTANTIATE_VARIABLE(Eigen::Vector3d)
INSTANTIATE_VARIABLE(sparse_container<double>)
INSTANTIATE_VARIABLE(sparse_container<float>)
INSTANTIATE_VARIABLE(sparse_container<int64_t>)
INSTANTIATE_VARIABLE(sparse_container<int32_t>)
// Some sparse instantiations are only needed to avoid linker errors: Some
// makeVariable overloads have a runtime branch that may instantiate a sparse
// variable.
INSTANTIATE_VARIABLE(sparse_container<std::string>)
INSTANTIATE_VARIABLE(sparse_container<bool>)
INSTANTIATE_VARIABLE(sparse_container<Eigen::Vector3d>)

INSTANTIATE_SET_VARIANCES(double)
INSTANTIATE_SET_VARIANCES(float)
INSTANTIATE_SET_VARIANCES(int64_t)
INSTANTIATE_SET_VARIANCES(int32_t)

} // namespace scipp::core
