// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/transform_common.h"
#include "scipp/units/except.h"

namespace scipp::core::element {

constexpr auto add_inplace_types =
    arg_list<double, float, int64_t, int32_t, Eigen::Vector3d, SubbinSizes,
             std::tuple<scipp::core::time_point, int64_t>,
             std::tuple<scipp::core::time_point, int32_t>,
             std::tuple<double, float>, std::tuple<float, double>,
             std::tuple<int64_t, int32_t>, std::tuple<int32_t, int64_t>,
             std::tuple<double, int64_t>, std::tuple<double, int32_t>,
             std::tuple<float, int64_t>, std::tuple<float, int32_t>,
             std::tuple<int64_t, bool>>;

constexpr auto add_equals =
    overloaded{add_inplace_types, [](auto &&a, const auto &b) { a += b; }};

constexpr auto nan_add_equals =
    overloaded{add_inplace_types, [](auto &&a, const auto &b) {
                 using numeric::isnan;
                 if (isnan(a))
                   a = std::decay_t<decltype(a)>{0}; // Force zero
                 if (!isnan(b))
                   a += b;
               }};

constexpr auto subtract_equals =
    overloaded{add_inplace_types, [](auto &&a, const auto &b) { a -= b; }};

constexpr auto mul_inplace_types = arg_list<
    double, float, int64_t, int32_t, Eigen::Matrix3d, std::tuple<double, float>,
    std::tuple<float, double>, std::tuple<int64_t, int32_t>,
    std::tuple<int64_t, bool>, std::tuple<int32_t, int64_t>,
    std::tuple<double, int64_t>, std::tuple<double, int32_t>,
    std::tuple<float, int64_t>, std::tuple<float, int32_t>,
    std::tuple<Eigen::Vector3d, double>, std::tuple<Eigen::Vector3d, float>,
    std::tuple<Eigen::Vector3d, int64_t>, std::tuple<Eigen::Vector3d, int32_t>>;

// Note that we do *not* support any integer type as left-hand-side, to match
// Python 3 and numpy "truediv" behavior. If "floordiv" is required it should be
// implemented as a separate operation.
constexpr auto div_inplace_types = arg_list<
    double, float, std::tuple<double, float>, std::tuple<float, double>,
    std::tuple<double, int64_t>, std::tuple<double, int32_t>,
    std::tuple<float, int64_t>, std::tuple<float, int32_t>,
    std::tuple<Eigen::Vector3d, double>, std::tuple<Eigen::Vector3d, float>,
    std::tuple<Eigen::Vector3d, int64_t>, std::tuple<Eigen::Vector3d, int32_t>>;

constexpr auto multiply_equals =
    overloaded{mul_inplace_types, [](auto &&a, const auto &b) { a *= b; }};
constexpr auto divide_equals =
    overloaded{div_inplace_types, [](auto &&a, const auto &b) { a /= b; }};

using arithmetic_and_matrix_type_pairs = decltype(
    std::tuple_cat(std::declval<arithmetic_type_pairs>(),
                   std::tuple<std::tuple<Eigen::Vector3d, Eigen::Vector3d>>()));

struct add_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(std::tuple_cat(
      std::declval<arithmetic_and_matrix_type_pairs>(),
      std::tuple<
          std::tuple<time_point, int64_t>, std::tuple<time_point, int32_t>,
          std::tuple<int64_t, time_point>, std::tuple<int32_t, time_point>>{}));
};

struct subtract_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(
      std::tuple_cat(std::declval<arithmetic_and_matrix_type_pairs>(),
                     std::tuple<std::tuple<time_point, int64_t>,
                                std::tuple<time_point, int32_t>,
                                std::tuple<time_point, time_point>>{}));
};

struct multiplies_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(std::tuple_cat(
      std::declval<arithmetic_type_pairs_with_bool>(),
      std::tuple<std::tuple<double, Eigen::Vector3d>>(),
      std::tuple<std::tuple<float, Eigen::Vector3d>>(),
      std::tuple<std::tuple<int64_t, Eigen::Vector3d>>(),
      std::tuple<std::tuple<int32_t, Eigen::Vector3d>>(),
      std::tuple<std::tuple<Eigen::Vector3d, double>>(),
      std::tuple<std::tuple<Eigen::Vector3d, float>>(),
      std::tuple<std::tuple<Eigen::Vector3d, int64_t>>(),
      std::tuple<std::tuple<Eigen::Vector3d, int32_t>>(),
      std::declval<
          pair_product_t<Eigen::Matrix3d, Eigen::Affine3d,
                         scipp::core::Quaternion, scipp::core::Translation>>(),
      std::tuple<std::tuple<scipp::core::Quaternion, Eigen::Vector3d>>(),
      std::tuple<std::tuple<Eigen::Matrix3d, Eigen::Vector3d>>()));
};

struct apply_spatial_transformation_t {
  constexpr void operator()() const noexcept;
  using types = decltype(std::tuple_cat(
      std::tuple<std::tuple<Eigen::Affine3d, Eigen::Vector3d>>(),
      std::tuple<std::tuple<scipp::core::Translation, Eigen::Vector3d>>(),

      std::tuple<
          std::tuple<scipp::core::Translation, scipp::core::Translation>>(),
      std::tuple<std::tuple<scipp::core::Translation, Eigen::Affine3d>>(),

      std::tuple<std::tuple<Eigen::Affine3d, Eigen::Affine3d>>(),
      std::tuple<std::tuple<Eigen::Affine3d, scipp::core::Translation>>()));
};

struct true_divide_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(
      std::tuple_cat(std::declval<arithmetic_type_pairs>(),
                     std::tuple<std::tuple<Eigen::Vector3d, double>>(),
                     std::tuple<std::tuple<Eigen::Vector3d, float>>(),
                     std::tuple<std::tuple<Eigen::Vector3d, int64_t>>(),
                     std::tuple<std::tuple<Eigen::Vector3d, int32_t>>()));
};

struct floor_divide_types_t {
  constexpr void operator()() const noexcept;
  using types = arithmetic_type_pairs;
};

struct remainder_types_t {
  constexpr void operator()() const noexcept;
  using types = arithmetic_type_pairs;
};

constexpr auto add =
    overloaded{add_types_t{}, [](const auto a, const auto b) { return a + b; }};
constexpr auto subtract = overloaded{
    subtract_types_t{}, [](const auto a, const auto b) { return a - b; }};
constexpr auto multiply = overloaded{
    multiplies_types_t{},
    transform_flags::expect_no_in_variance_if_out_cannot_have_variance,
    [](const auto a, const auto b) { return a * b; }};

constexpr auto apply_spatial_transformation = overloaded{
    apply_spatial_transformation_t{},
    transform_flags::expect_no_in_variance_if_out_cannot_have_variance,
    [](const auto a, const auto b) { return a * b; },
    [](const units::Unit &a, const units::Unit &b) {
      if (a != b)
        throw except::UnitError(
            "Cannot apply spatial transform as the units of the transformation "
            "are not the same as the units of transformation or vector.");
      else
        return a;
    }};

// truediv defined as in Python.
constexpr auto divide = overloaded{
    true_divide_types_t{},
    transform_flags::expect_no_in_variance_if_out_cannot_have_variance,
    [](const auto &a, const auto &b) { return numeric::true_divide(a, b); },
    [](const units::Unit &a, const units::Unit &b) { return a / b; }};

// floordiv defined as in Python. Complementary to mod.
constexpr auto floor_divide = overloaded{
    floor_divide_types_t{}, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const auto a, const auto b) { return numeric::floor_divide(a, b); },
    [](const units::Unit &a, const units::Unit &b) { return a / b; }};

// remainder defined as in Python
constexpr auto mod = overloaded{
    remainder_types_t{}, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const auto a, const auto b) { return numeric::remainder(a, b); },
    [](const units::Unit &a, const units::Unit &b) { return a % b; }};

constexpr auto mod_equals =
    overloaded{arg_list<int64_t, int32_t, std::tuple<int64_t, int32_t>>,
               [](units::Unit &a, const units::Unit &b) { a %= b; },
               [](auto &&a, const auto &b) { a = mod(a, b); }};

constexpr auto unary_minus =
    overloaded{arg_list<double, float, int64_t, int32_t, Eigen::Vector3d>,
               [](const auto x) { return -x; }};

} // namespace scipp::core::element
