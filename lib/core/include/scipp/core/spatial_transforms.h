// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/core/dtype.h"

namespace scipp::core {

class eigen_rotation_type {
public:
    Eigen::Matrix3d mat;
};

class eigen_scaling_type {
public:
    Eigen::Matrix3d mat;
};

class eigen_translation_type {
public:
    Eigen::Vector3d vec;
};

[[nodiscard]] inline eigen_rotation_type operator*(const eigen_rotation_type &lhs, const eigen_rotation_type &rhs) {
    return eigen_rotation_type{(lhs.mat * rhs.mat).eval()};
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const eigen_rotation_type &lhs, const eigen_scaling_type &rhs) {
    return (lhs.mat * rhs.mat).eval();
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const eigen_rotation_type &lhs, const Eigen::Matrix3d &rhs) {
    return (lhs.mat * rhs).eval();
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_rotation_type &lhs, const eigen_translation_type &rhs) {
    return (lhs.mat * Eigen::Translation<double, 3>(rhs.vec));
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_rotation_type &lhs, const Eigen::Affine3d &rhs) {
    return lhs.mat * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const eigen_rotation_type &lhs, const Eigen::Vector3d &rhs) {
    return lhs.mat * rhs;
};



[[nodiscard]] inline eigen_scaling_type operator*(const eigen_scaling_type &lhs, const eigen_scaling_type &rhs) {
    return eigen_scaling_type{(lhs.mat * rhs.mat).eval()};
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const eigen_scaling_type &lhs, const eigen_rotation_type &rhs) {
    return (lhs.mat * rhs.mat).eval();
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const eigen_scaling_type &lhs, const Eigen::Matrix3d &rhs) {
    return (lhs.mat * rhs).eval();
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_scaling_type &lhs, const eigen_translation_type &rhs) {
    return (lhs.mat * Eigen::Translation<double, 3>(rhs.vec));
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_scaling_type &lhs, const Eigen::Affine3d &rhs) {
    return lhs.mat * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const eigen_scaling_type &lhs, const Eigen::Vector3d &rhs) {
    return lhs.mat * rhs;
};



[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_translation_type &lhs, const eigen_scaling_type &rhs) {
    return Eigen::Translation<double, 3>(lhs.vec) * rhs.mat;
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_translation_type &lhs, const eigen_rotation_type &rhs) {
    return Eigen::Translation<double, 3>(lhs.vec) * rhs.mat;
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_translation_type &lhs, const Eigen::Matrix3d &rhs) {
    return Eigen::Translation<double, 3>(lhs.vec) * rhs;
};

[[nodiscard]] inline eigen_translation_type operator*(const eigen_translation_type &lhs, const eigen_translation_type &rhs) {
    return eigen_translation_type{(lhs.vec + rhs.vec).eval()};
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_translation_type &lhs, const Eigen::Affine3d &rhs) {
    return Eigen::Translation<double, 3>(lhs.vec) * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const eigen_translation_type &lhs, const Eigen::Vector3d &rhs) {
    return lhs.vec + rhs;
};



[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_translation_type &rhs) {
    return lhs * Eigen::Translation<double, 3>(rhs.vec);
}

[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_rotation_type &rhs) {
    return Eigen::Affine3d(lhs * rhs.mat);
}

[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_scaling_type &rhs) {
    return Eigen::Affine3d(lhs * rhs.mat);
}



[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Matrix3d &lhs, const eigen_translation_type &rhs) {
    return lhs * Eigen::Translation<double, 3>(rhs.vec);
}

[[nodiscard]] inline Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs, const eigen_rotation_type &rhs) {
    return lhs * rhs.mat;
}

[[nodiscard]] inline Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs, const eigen_scaling_type &rhs) {
    return lhs * rhs.mat;
}

template <> inline constexpr DType dtype<Eigen::Matrix3d>{5001};
template <> inline constexpr DType dtype<Eigen::Affine3d>{5002};
template <> inline constexpr DType dtype<eigen_translation_type>{5003};
template <> inline constexpr DType dtype<eigen_scaling_type>{5004};
template <> inline constexpr DType dtype<eigen_rotation_type>{5005};

} // namespace scipp::core
