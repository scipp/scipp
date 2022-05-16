// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/common/numeric.h"
#include "scipp/core/dtype.h"
#include "scipp/core/spatial_transforms.h"

namespace scipp::core {

template <> inline constexpr DType dtype<Eigen::Vector3d>{4000};
// 4001-4004 defined in spatial_transforms.h
template <>
inline constexpr DType dtype<scipp::span<const Eigen::Vector3d>>{4100};
template <> inline constexpr DType dtype<scipp::span<Eigen::Vector3d>>{4200};

} // namespace scipp::core

namespace scipp::numeric {

template <> inline bool isnan<Eigen::Vector3d>(const Eigen::Vector3d &x) {
  return x.hasNaN();
}

template <> inline bool isfinite<Eigen::Vector3d>(const Eigen::Vector3d &x) {
  return x.allFinite();
}

template <> inline bool isinf<Eigen::Vector3d>(const Eigen::Vector3d &x) {
  return !isfinite(x) && !isnan(x);
}

} // namespace scipp::numeric
