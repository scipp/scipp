// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/core/dtype.h"

#include <optional>
#include <type_traits>

namespace scipp::core {

class Quaternion {
private:
  // Store as quaterniond as this is more space efficient than storing as matrix
  // (4 doubles for quat vs 9 doubles for 3x3 matrix).
  Eigen::Quaterniond m_quat;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Quaternion() : m_quat(Eigen::Quaterniond::Identity()){};
  explicit Quaternion(const Eigen::Quaterniond &x) : m_quat(x){};

  [[nodiscard]] const Eigen::Quaterniond &quat() const { return m_quat; }

  bool operator==(const Quaternion &other) const {
    return m_quat.w() == other.m_quat.w() && m_quat.x() == other.m_quat.x() &&
           m_quat.y() == other.m_quat.y() && m_quat.z() == other.m_quat.z();
  }

  double &operator()(const int i) {
    if (i == 0) {
      return m_quat.x();
    } else if (i == 1) {
      return m_quat.y();
    } else if (i == 2) {
      return m_quat.z();
    } else if (i == 3) {
      return m_quat.w();
    } else {
      throw std::out_of_range("invalid index for Quaternion");
    }
  }
};

class Translation {
private:
  Eigen::Vector3d m_vec;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Translation() : m_vec(Eigen::Vector3d(0, 0, 0)){};
  explicit Translation(const Eigen::Vector3d &x) : m_vec(x){};

  [[nodiscard]] const Eigen::Vector3d &vector() const { return m_vec; }

  bool operator==(const Translation &other) const {
    return m_vec == other.m_vec;
  }

  double &operator()(const int i) { return m_vec(i); }
};

template <typename T> inline const T &asEigenType(const T &obj) { return obj; };
inline const auto &asEigenType(const Quaternion &obj) { return obj.quat(); }
inline auto asEigenType(const Translation &obj) {
  return Eigen::Translation<double, 3>(obj.vector());
}

template <class T_LHS, class T_RHS>
struct combines_to_linear : std::false_type {};
template <>
struct combines_to_linear<Quaternion, Eigen::Matrix3d> : std::true_type {};
template <>
struct combines_to_linear<Eigen::Matrix3d, Quaternion> : std::true_type {};

template <typename T_LHS, typename T_RHS>
[[nodiscard]] inline std::enable_if_t<combines_to_linear<T_LHS, T_RHS>::value,
                                      Eigen::Matrix3d>
operator*(const T_LHS &lhs, const T_RHS &rhs) {
  return asEigenType(lhs) * asEigenType(rhs);
}

template <class T_LHS, class T_RHS>
struct combines_to_affine : std::false_type {};
template <>
struct combines_to_affine<Quaternion, Translation> : std::true_type {};
template <>
struct combines_to_affine<Quaternion, Eigen::Affine3d> : std::true_type {};
template <>
struct combines_to_affine<Eigen::Matrix3d, Translation> : std::true_type {};
template <>
struct combines_to_affine<Eigen::Affine3d, Quaternion> : std::true_type {};
template <>
struct combines_to_affine<Eigen::Affine3d, Translation> : std::true_type {};
template <>
struct combines_to_affine<Translation, Quaternion> : std::true_type {};
template <>
struct combines_to_affine<Translation, Eigen::Matrix3d> : std::true_type {};
template <>
struct combines_to_affine<Translation, Eigen::Affine3d> : std::true_type {};

template <typename T_LHS, typename T_RHS>
[[nodiscard]] inline std::enable_if_t<combines_to_affine<T_LHS, T_RHS>::value,
                                      Eigen::Affine3d>
operator*(const T_LHS &lhs, const T_RHS &rhs) {
  return Eigen::Affine3d(asEigenType(lhs) * asEigenType(rhs));
}

[[nodiscard]] inline Eigen::Vector3d operator*(const Quaternion &lhs,
                                               const Eigen::Vector3d &rhs) {
  return asEigenType(lhs) * rhs;
}

[[nodiscard]] inline Eigen::Vector3d operator*(const Translation &lhs,
                                               const Eigen::Vector3d &rhs) {
  return asEigenType(lhs) * rhs;
}

[[nodiscard]] inline Quaternion operator*(const Quaternion &lhs,
                                          const Quaternion &rhs) {
  return Quaternion(lhs.quat() * rhs.quat());
};

[[nodiscard]] inline Translation operator*(const Translation &lhs,
                                           const Translation &rhs) {
  return Translation(lhs.vector() + rhs.vector());
};

template <> inline constexpr DType dtype<Eigen::Matrix3d>{4001};
template <> inline constexpr DType dtype<Eigen::Affine3d>{4002};
template <> inline constexpr DType dtype<Translation>{4003};
template <> inline constexpr DType dtype<Quaternion>{4004};

} // namespace scipp::core
