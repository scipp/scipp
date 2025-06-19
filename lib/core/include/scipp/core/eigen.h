// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

// Warnings are raised by eigen headers with gcc12
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include <Eigen/Core>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <Eigen/Geometry>

#include "scipp/common/numeric.h"
#include "scipp/core/bucket.h"
#include "scipp/core/dtype.h"
#include "scipp/core/spatial_transforms.h"

namespace scipp::core {

template <> inline constexpr DType dtype<Eigen::Vector3d>{4000};
// 4001-4004 defined in spatial_transforms.h
template <>
inline constexpr DType dtype<std::span<const Eigen::Vector3d>>{4100};
template <> inline constexpr DType dtype<std::span<Eigen::Vector3d>>{4200};

constexpr bool is_structured(DType tp) {
  return tp == dtype<Eigen::Vector3d> || tp == dtype<Eigen::Matrix3d> ||
         tp == dtype<scipp::core::Quaternion> || tp == dtype<Eigen::Affine3d> ||
         tp == dtype<scipp::core::Translation> || tp == dtype<index_pair>;
}

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

[[nodiscard]] inline bool operator==(const Eigen::Affine3d &a,
                                     const Eigen::Affine3d &b) {
  return a.matrix() == b.matrix();
}

[[nodiscard]] inline bool operator!=(const Eigen::Affine3d &a,
                                     const Eigen::Affine3d &b) {
  return a.matrix() != b.matrix();
}

} // namespace scipp::numeric
