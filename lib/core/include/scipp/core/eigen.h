// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/core/dtype.h"
#include "scipp/core/spatial_transforms.h"

namespace scipp::core {

template <> inline constexpr DType dtype<Eigen::Vector3d>{4000};
// 4001-4004 defined in spatial_transforms.h
template <>
inline constexpr DType dtype<scipp::span<const Eigen::Vector3d>>{4100};
template <> inline constexpr DType dtype<scipp::span<Eigen::Vector3d>>{4200};

} // namespace scipp::core
