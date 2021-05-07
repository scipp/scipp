// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>

#include "scipp/core/dtype.h"

namespace scipp::core {

template <> inline constexpr DType dtype<Eigen::Vector3d>{4000};
template <> inline constexpr DType dtype<Eigen::Matrix3d>{4001};
template <> inline constexpr DType dtype<span<const Eigen::Vector3d>>{4100};
template <> inline constexpr DType dtype<span<Eigen::Vector3d>>{4200};

} // namespace scipp::core
