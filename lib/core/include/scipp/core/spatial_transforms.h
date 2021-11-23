// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/core/dtype.h"

namespace scipp::core {

class eigen_rotation_type;
class eigen_scaling_type;
class eigen_translation_type;

class eigen_rotation_type {
public:
    Eigen::Matrix3d mat;

    SCIPP_CORE_EXPORT eigen_rotation_type operator*(const eigen_rotation_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Matrix3d operator*(const eigen_scaling_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Matrix3d operator*(const Eigen::Matrix3d &other) const;
    SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const eigen_translation_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &other) const;
    SCIPP_CORE_EXPORT Eigen::Vector3d operator*(const Eigen::Vector3d &other) const;
};

class eigen_scaling_type {
public:
    Eigen::Matrix3d mat;

    SCIPP_CORE_EXPORT eigen_scaling_type operator*(const eigen_scaling_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Matrix3d operator*(const eigen_rotation_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Matrix3d operator*(const Eigen::Matrix3d &other) const;
    SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const eigen_translation_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &other) const;
    SCIPP_CORE_EXPORT Eigen::Vector3d operator*(const Eigen::Vector3d &other) const;
};

class eigen_translation_type {
public:
    Eigen::Vector3d vec;

    SCIPP_CORE_EXPORT eigen_translation_type operator*(const eigen_translation_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const eigen_rotation_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const eigen_scaling_type &other) const;
    SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Matrix3d &other) const;
    SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &other) const;
    SCIPP_CORE_EXPORT Eigen::Vector3d operator*(const Eigen::Vector3d &other) const;
};

SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_translation_type &rhs);
SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_rotation_type &rhs);
SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_scaling_type &rhs);

SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Matrix3d &lhs, const eigen_translation_type &rhs);
SCIPP_CORE_EXPORT Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs, const eigen_rotation_type &rhs);
SCIPP_CORE_EXPORT Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs, const eigen_scaling_type &rhs);

template <> inline constexpr DType dtype<Eigen::Matrix3d>{5001};
template <> inline constexpr DType dtype<Eigen::Affine3d>{5002};
template <> inline constexpr DType dtype<eigen_translation_type>{5003};
template <> inline constexpr DType dtype<eigen_scaling_type>{5004};
template <> inline constexpr DType dtype<eigen_rotation_type>{5005};

} // namespace scipp::core
