// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/core/dtype.h"

namespace scipp::core {

using eigen_translation_type = Eigen::Translation<double, 3>;
using eigen_scaling_type = Eigen::DiagonalMatrix<double, 3, 3>;
using eigen_rotation_type = Eigen::Quaterniond;

template <> inline constexpr DType dtype<Eigen::Vector3d>{4000};
template <> inline constexpr DType dtype<Eigen::Matrix3d>{4001};
template <> inline constexpr DType dtype<Eigen::Affine3d>{4002};
template <> inline constexpr DType dtype<eigen_translation_type>{4003};
template <> inline constexpr DType dtype<eigen_scaling_type>{4004};
template <> inline constexpr DType dtype<eigen_rotation_type>{4005};
template <>
inline constexpr DType dtype<std::span<const Eigen::Vector3d>>{4100};
template <> inline constexpr DType dtype<std::span<Eigen::Vector3d>>{4200};

} // namespace scipp::core
