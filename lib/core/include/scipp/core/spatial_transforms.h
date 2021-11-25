// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <iostream>

#include "scipp/core/dtype.h"

namespace scipp::core {

class eigen_rotation_type {
private:
    // Store as quaterniond as this is more space efficient than storing as matrix
    // (4 doubles for quat vs 9 doubles for 3x3 matrix).
    Eigen::Quaterniond quat;
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    eigen_rotation_type() : quat(Eigen::Quaterniond::Identity()) { };
    explicit eigen_rotation_type(const Eigen::Quaterniond &x) : quat(x) { };

    [[nodiscard]] Eigen::Matrix3d matrix() const {
        return quat.toRotationMatrix();
    }

    inline bool operator==(const eigen_rotation_type &other) const {
        return this->matrix() == other.matrix();
    }

    const eigen_rotation_type& eval() const {
        return *this;
    };
};

class eigen_scaling_type {
private:
    Eigen::DiagonalMatrix<double, 3> mat;
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    eigen_scaling_type() : mat(Eigen::Matrix3d::Identity().diagonal().asDiagonal()) { };
    explicit eigen_scaling_type(const Eigen::DiagonalMatrix<double, 3> &x) : mat(x) { };

    [[nodiscard]] Eigen::Matrix3d matrix() const {
        return mat;
    }

    inline bool operator==(const eigen_scaling_type &other) const {
        return this->matrix() == other.matrix();
    }

    const eigen_scaling_type& eval() const {
        return *this;
    };
};

class eigen_translation_type {
private:
    Eigen::Vector3d vec;
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    eigen_translation_type() : vec(Eigen::Vector3d(0, 0, 0)) { };
    explicit eigen_translation_type(const Eigen::Vector3d &x) : vec(x) { };

    [[nodiscard]] Eigen::Vector3d vector() const {
        return vec;
    }

    inline bool operator==(const eigen_translation_type &other) const {
        return this->vector() == other.vector();
    }

    const eigen_translation_type& eval() const {
        return *this;
    };
};

[[nodiscard]] inline eigen_rotation_type operator*(const eigen_rotation_type &lhs, const eigen_rotation_type &rhs) {
    return eigen_rotation_type(Eigen::Quaterniond(lhs.matrix() * rhs.matrix()));
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const eigen_rotation_type &lhs, const eigen_scaling_type &rhs) {
    return lhs.matrix() * rhs.matrix();
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const eigen_rotation_type &lhs, const Eigen::Matrix3d &rhs) {
    return lhs.matrix() * rhs;
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_rotation_type &lhs, const eigen_translation_type &rhs) {
    return lhs.matrix() * Eigen::Translation<double, 3>(rhs.vector());
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_rotation_type &lhs, const Eigen::Affine3d &rhs) {
    return lhs.matrix() * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const eigen_rotation_type &lhs, const Eigen::Vector3d &rhs) {
    return lhs.matrix() * rhs;
};



[[nodiscard]] inline eigen_scaling_type operator*(const eigen_scaling_type &lhs, const eigen_scaling_type &rhs) {
    return eigen_scaling_type((lhs.matrix() * rhs.matrix()).diagonal().asDiagonal());
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const eigen_scaling_type &lhs, const eigen_rotation_type &rhs) {
    return lhs.matrix() * rhs.matrix();
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const eigen_scaling_type &lhs, const Eigen::Matrix3d &rhs) {
    return lhs.matrix() * rhs;
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_scaling_type &lhs, const eigen_translation_type &rhs) {
    return lhs.matrix() * Eigen::Translation<double, 3>(rhs.vector());
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_scaling_type &lhs, const Eigen::Affine3d &rhs) {
    return lhs.matrix() * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const eigen_scaling_type &lhs, const Eigen::Vector3d &rhs) {
    return lhs.matrix() * rhs;
};



[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_translation_type &lhs, const eigen_scaling_type &rhs) {
    return Eigen::Translation<double, 3>(lhs.vector()) * rhs.matrix();
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_translation_type &lhs, const eigen_rotation_type &rhs) {
    return Eigen::Translation<double, 3>(lhs.vector()) * rhs.matrix();
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_translation_type &lhs, const Eigen::Matrix3d &rhs) {
    return Eigen::Translation<double, 3>(lhs.vector()) * rhs;
};

[[nodiscard]] inline eigen_translation_type operator*(const eigen_translation_type &lhs, const eigen_translation_type &rhs) {
    return eigen_translation_type(lhs.vector() + rhs.vector());
};

[[nodiscard]] inline Eigen::Affine3d operator*(const eigen_translation_type &lhs, const Eigen::Affine3d &rhs) {
    return Eigen::Translation<double, 3>(lhs.vector()) * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const eigen_translation_type &lhs, const Eigen::Vector3d &rhs) {
    return lhs.vector() + rhs;
};



[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_translation_type &rhs) {
    return lhs * Eigen::Translation<double, 3>(rhs.vector());
}

[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_rotation_type &rhs) {
    return Eigen::Affine3d(lhs * rhs.matrix());
}

[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_scaling_type &rhs) {
    return Eigen::Affine3d(lhs * rhs.matrix());
}



[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Matrix3d &lhs, const eigen_translation_type &rhs) {
    return lhs * Eigen::Translation<double, 3>(rhs.vector());
}

[[nodiscard]] inline Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs, const eigen_rotation_type &rhs) {
    return lhs * rhs.matrix();
}

[[nodiscard]] inline Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs, const eigen_scaling_type &rhs) {
    return lhs * rhs.matrix();
}

template <> inline constexpr DType dtype<Eigen::Matrix3d>{5001};
template <> inline constexpr DType dtype<Eigen::Affine3d>{5002};
template <> inline constexpr DType dtype<eigen_translation_type>{5003};
template <> inline constexpr DType dtype<eigen_scaling_type>{5004};
template <> inline constexpr DType dtype<eigen_rotation_type>{5005};

} // namespace scipp::core
