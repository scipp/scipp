// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/core/dtype.h"

namespace scipp::core {

class Rotation {
private:
  // Store as quaterniond as this is more space efficient than storing as matrix
  // (4 doubles for quat vs 9 doubles for 3x3 matrix).
  Eigen::Quaterniond m_quat;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Rotation() : m_quat(Eigen::Quaterniond::Identity()){};
  explicit Rotation(const Eigen::Quaterniond &x) : m_quat(x){};

  [[nodiscard]] Eigen::Quaterniond quat() const {
    return m_quat;
  }

  bool operator==(const Rotation &other) const {
    return m_quat.w() == other.m_quat.w() && 
        m_quat.x() == other.m_quat.x() && 
        m_quat.y() == other.m_quat.y() && 
        m_quat.z() == other.m_quat.z();
  }

  const double get(const int index) const {
      if (index == 0)
          return m_quat.w();
      else if (index == 1)
          return m_quat.x();
      else if (index == 2)
          return m_quat.y();
      else if (index == 3)
          return m_quat.z();
      else
          throw std::out_of_range("invalid index in get for Rotation");
  }
};

class Scaling {
private:
  Eigen::DiagonalMatrix<double, 3> m_mat;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Scaling()
      : m_mat(Eigen::Matrix3d::Identity().diagonal().asDiagonal()){};
  explicit Scaling(const Eigen::DiagonalMatrix<double, 3> &x)
      : m_mat(x){};

  [[nodiscard]] Eigen::Matrix3d matrix() const { return m_mat; }

  bool operator==(const Scaling &other) const {
    return m_mat.diagonal() == other.m_mat.diagonal();
  }

  const double get(const int index) const {
      return m_mat.diagonal()(index);
  }
};

class Translation {
private:
  Eigen::Vector3d m_vec;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Translation() : m_vec(Eigen::Vector3d(0, 0, 0)){};
  explicit Translation(const Eigen::Vector3d &x) : m_vec(x){};

  [[nodiscard]] Eigen::Vector3d vector() const { return m_vec; }

  bool operator==(const Translation &other) const {
    return m_vec == other.m_vec;
  }

  const double get(const int index) const {
      return m_vec(index);
  }

  const Eigen::Translation<double, 3> asEigenTranslation() const {
      return Eigen::Translation<double, 3>(m_vec);
  }
};

[[nodiscard]] inline Rotation operator*(const Rotation &lhs,
                                                 const Rotation &rhs) {
  return Rotation(lhs.quat() * rhs.quat());
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const Rotation &lhs,
                                               const Scaling &rhs) {
  return lhs.quat() * rhs.matrix();
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const Rotation &lhs,
                                               const Eigen::Matrix3d &rhs) {
  return lhs.quat() * rhs;
};

[[nodiscard]] inline Eigen::Affine3d
operator*(const Rotation &lhs, const Translation &rhs) {
  return lhs.quat() * rhs.asEigenTranslation();
};

[[nodiscard]] inline Eigen::Affine3d operator*(const Rotation &lhs,
                                               const Eigen::Affine3d &rhs) {
  return lhs.quat() * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const Rotation &lhs,
                                               const Eigen::Vector3d &rhs) {
  return lhs.quat() * rhs;
};

[[nodiscard]] inline Scaling operator*(const Scaling &lhs,
                                                const Scaling &rhs) {
  return Scaling(
      (lhs.matrix() * rhs.matrix()).diagonal().asDiagonal());
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const Scaling &lhs,
                                               const Rotation &rhs) {
  return lhs.matrix() * rhs.quat();
};

[[nodiscard]] inline Eigen::Matrix3d operator*(const Scaling &lhs,
                                               const Eigen::Matrix3d &rhs) {
  return lhs.matrix() * rhs;
};

[[nodiscard]] inline Eigen::Affine3d
operator*(const Scaling &lhs, const Translation &rhs) {
  return lhs.matrix() * rhs.asEigenTranslation();
};

[[nodiscard]] inline Eigen::Affine3d operator*(const Scaling &lhs,
                                               const Eigen::Affine3d &rhs) {
  return lhs.matrix() * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const Scaling &lhs,
                                               const Eigen::Vector3d &rhs) {
  return lhs.matrix() * rhs;
};

[[nodiscard]] inline Eigen::Affine3d operator*(const Translation &lhs,
                                               const Scaling &rhs) {
  return lhs.asEigenTranslation() * rhs.matrix();
};

[[nodiscard]] inline Eigen::Affine3d operator*(const Translation &lhs,
                                               const Rotation &rhs) {
  return lhs.asEigenTranslation() * rhs.quat();
};

[[nodiscard]] inline Eigen::Affine3d operator*(const Translation &lhs,
                                               const Eigen::Matrix3d &rhs) {
  return lhs.asEigenTranslation() * rhs;
};

[[nodiscard]] inline Translation
operator*(const Translation &lhs, const Translation &rhs) {
  return Translation(lhs.vector() + rhs.vector());
};

[[nodiscard]] inline Eigen::Affine3d operator*(const Translation &lhs,
                                               const Eigen::Affine3d &rhs) {
  return lhs.asEigenTranslation() * rhs;
};

[[nodiscard]] inline Eigen::Vector3d operator*(const Translation &lhs,
                                               const Eigen::Vector3d &rhs) {
  return lhs.vector() + rhs;
};

[[nodiscard]] inline Eigen::Affine3d
operator*(const Eigen::Affine3d &lhs, const Translation &rhs) {
  return lhs * rhs.asEigenTranslation();
}

[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Affine3d &lhs,
                                               const Rotation &rhs) {
  return Eigen::Affine3d(lhs * rhs.quat());
}

[[nodiscard]] inline Eigen::Affine3d operator*(const Eigen::Affine3d &lhs,
                                               const Scaling &rhs) {
  return Eigen::Affine3d(lhs * rhs.matrix());
}

[[nodiscard]] inline Eigen::Affine3d
operator*(const Eigen::Matrix3d &lhs, const Translation &rhs) {
  return lhs * rhs.asEigenTranslation();
}

[[nodiscard]] inline Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs,
                                               const Rotation &rhs) {
  return lhs * rhs.quat();
}

[[nodiscard]] inline Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs,
                                               const Scaling &rhs) {
  return lhs * rhs.matrix();
}

template <> inline constexpr DType dtype<Eigen::Matrix3d>{5001};
template <> inline constexpr DType dtype<Eigen::Affine3d>{5002};
template <> inline constexpr DType dtype<Translation>{5003};
template <> inline constexpr DType dtype<Scaling>{5004};
template <> inline constexpr DType dtype<Rotation>{5005};

} // namespace scipp::core
