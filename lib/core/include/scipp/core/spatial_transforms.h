// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/core/dtype.h"

#include <type_traits>
#include <optional>

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

  double& operator()(const int i) {
      if (i == 0) {
           m_quat.x();
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

  double& operator()(const int i) {
      return m_mat.diagonal()(i);
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

  double& operator()(const int i) {
      return m_vec(i);
  } 
};

template<class T> struct asEigenType_t { using type = T; };
template<> struct asEigenType_t<Rotation> { using type = Eigen::Quaterniond; };
template<> struct asEigenType_t<Scaling> { using type = Eigen::Matrix3d; };
template<> struct asEigenType_t<Translation> { using type = Eigen::Translation<double, 3>; };

template<typename T, typename R = asEigenType_t<T>::type> inline const R asEigenType(const T &obj) { return obj; };
template<> inline const Eigen::Quaterniond asEigenType(const Rotation &obj) { return obj.quat(); }
template<> inline const Eigen::Matrix3d asEigenType(const Scaling &obj) { return obj.matrix(); }
template<> inline const Eigen::Translation<double, 3> asEigenType(const Translation &obj) { return Eigen::Translation<double, 3>(obj.vector()); }

template<class T_LHS, class T_RHS> struct combines_to_linear : std::false_type {}; 
template<> struct combines_to_linear<Rotation, Scaling> : std::true_type {};
template<> struct combines_to_linear<Rotation, Eigen::Matrix3d> : std::true_type {};
template<> struct combines_to_linear<Scaling, Rotation> : std::true_type {};
template<> struct combines_to_linear<Scaling, Eigen::Matrix3d> : std::true_type {};
template<> struct combines_to_linear<Eigen::Matrix3d, Rotation> : std::true_type {};
template<> struct combines_to_linear<Eigen::Matrix3d, Scaling> : std::true_type {};

template <typename T_LHS, typename T_RHS>
[[nodiscard]] inline 
std::enable_if_t<combines_to_linear<T_LHS, T_RHS>::value, Eigen::Matrix3d> 
operator*(const T_LHS &lhs, const T_RHS &rhs) {
    return asEigenType(lhs) * asEigenType(rhs);
}

template<class T_LHS, class T_RHS> struct combines_to_affine : std::false_type {}; 
template<> struct combines_to_affine<Rotation, Translation> : std::true_type {};
template<> struct combines_to_affine<Rotation, Eigen::Affine3d> : std::true_type {};
template<> struct combines_to_affine<Scaling, Translation> : std::true_type {};
template<> struct combines_to_affine<Scaling, Eigen::Affine3d> : std::true_type {};
template<> struct combines_to_affine<Eigen::Matrix3d, Translation> : std::true_type {};
template<> struct combines_to_affine<Eigen::Affine3d, Rotation> : std::true_type {};
template<> struct combines_to_affine<Eigen::Affine3d, Scaling> : std::true_type {};
template<> struct combines_to_affine<Eigen::Affine3d, Translation> : std::true_type {};
template<> struct combines_to_affine<Translation, Rotation> : std::true_type {};
template<> struct combines_to_affine<Translation, Scaling> : std::true_type {};
template<> struct combines_to_affine<Translation, Eigen::Matrix3d> : std::true_type {};
template<> struct combines_to_affine<Translation, Eigen::Affine3d> : std::true_type {};

template <typename T_LHS, typename T_RHS>
[[nodiscard]] inline 
std::enable_if_t<combines_to_affine<T_LHS, T_RHS>::value, Eigen::Affine3d> 
operator*(const T_LHS &lhs, const T_RHS &rhs) {
    return Eigen::Affine3d(asEigenType(lhs) * asEigenType(rhs));
}

template<class T_LHS> struct applies_to_vector : std::false_type {}; 
template<> struct applies_to_vector<Rotation> : std::true_type {};
template<> struct applies_to_vector<Translation> : std::true_type {};
template<> struct applies_to_vector<Scaling> : std::true_type {};

template <typename T_LHS>
[[nodiscard]] inline 
std::enable_if_t<applies_to_vector<T_LHS>::value, Eigen::Vector3d> 
operator*(const T_LHS &lhs, const Eigen::Vector3d &rhs) {
    return asEigenType(lhs) * rhs;
}

[[nodiscard]] inline Rotation operator*(const Rotation &lhs,
                                                 const Rotation &rhs) {
  return Rotation(lhs.quat() * rhs.quat());
};

[[nodiscard]] inline Scaling operator*(const Scaling &lhs,
                                                const Scaling &rhs) {
  return Scaling(
      (lhs.matrix() * rhs.matrix()).diagonal().asDiagonal());
};

[[nodiscard]] inline Translation
operator*(const Translation &lhs, const Translation &rhs) {
  return Translation(lhs.vector() + rhs.vector());
};

template <> inline constexpr DType dtype<Eigen::Matrix3d>{5001};
template <> inline constexpr DType dtype<Eigen::Affine3d>{5002};
template <> inline constexpr DType dtype<Translation>{5003};
template <> inline constexpr DType dtype<Scaling>{5004};
template <> inline constexpr DType dtype<Rotation>{5005};

} // namespace scipp::core
