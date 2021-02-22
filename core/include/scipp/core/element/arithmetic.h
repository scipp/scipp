// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <Eigen/Dense>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/transform_common.h"

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

constexpr auto plus_equals =
    overloaded{add_inplace_types, [](auto &&a, const auto &b) { a += b; }};

constexpr auto nan_plus_equals =
    overloaded{add_inplace_types, [](auto &&a, const auto &b) {
                 using numeric::isnan;
                 if (isnan(a))
                   a = std::decay_t<decltype(a)>{0}; // Force zero
                 if (!isnan(b))
                   a += b;
               }};

constexpr auto minus_equals =
    overloaded{add_inplace_types, [](auto &&a, const auto &b) { a -= b; }};

constexpr auto mul_inplace_types = arg_list<
    double, float, int64_t, int32_t, std::tuple<double, float>,
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

constexpr auto times_equals =
    overloaded{mul_inplace_types, [](auto &&a, const auto &b) { a *= b; }};
constexpr auto divide_equals =
    overloaded{div_inplace_types, [](auto &&a, const auto &b) { a /= b; }};

struct add_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(std::tuple_cat(
      std::declval<arithmetic_and_matrix_type_pairs>(),
      std::tuple<
          std::tuple<time_point, int64_t>, std::tuple<time_point, int32_t>,
          std::tuple<int64_t, time_point>, std::tuple<int32_t, time_point>>{}));
};

struct minus_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(
      std::tuple_cat(std::declval<arithmetic_and_matrix_type_pairs>(),
                     std::tuple<std::tuple<time_point, int64_t>,
                                std::tuple<time_point, int32_t>,
                                std::tuple<time_point, time_point>>{}));
};

struct times_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(
      std::tuple_cat(std::declval<arithmetic_type_pairs_with_bool>(),
                     std::tuple<std::tuple<Eigen::Matrix3d, Eigen::Matrix3d>>(),
                     std::tuple<std::tuple<Eigen::Matrix3d, Eigen::Vector3d>>(),
                     std::tuple<std::tuple<double, Eigen::Vector3d>>(),
                     std::tuple<std::tuple<float, Eigen::Vector3d>>(),
                     std::tuple<std::tuple<int64_t, Eigen::Vector3d>>(),
                     std::tuple<std::tuple<int32_t, Eigen::Vector3d>>(),
                     std::tuple<std::tuple<Eigen::Vector3d, double>>(),
                     std::tuple<std::tuple<Eigen::Vector3d, float>>(),
                     std::tuple<std::tuple<Eigen::Vector3d, int64_t>>(),
                     std::tuple<std::tuple<Eigen::Vector3d, int32_t>>()));
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

constexpr auto plus =
    overloaded{add_types_t{}, [](const auto a, const auto b) { return a + b; }};
constexpr auto minus = overloaded{
    minus_types_t{}, [](const auto a, const auto b) { return a - b; }};
constexpr auto times = overloaded{
    times_types_t{},
    transform_flags::expect_no_in_variance_if_out_cannot_have_variance,
    [](const auto a, const auto b) { return a * b; }};

// truediv defined as in Python.
constexpr auto divide = overloaded{
    true_divide_types_t{},
    transform_flags::expect_no_in_variance_if_out_cannot_have_variance,
    [](const auto a, const auto b) {
      if constexpr (std::is_integral_v<decltype(a)> &&
                    std::is_integral_v<decltype(b)>)
        return static_cast<double>(a) / static_cast<double>(b);
      else
        return a / b;
    }};

// floordiv defined as in Python. Complementary to mod.
constexpr auto floor_divide = overloaded{
    floor_divide_types_t{}, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const auto a,
       const auto b) -> std::common_type_t<decltype(a), decltype(b)> {
      using std::floor;
      if constexpr (std::is_integral_v<decltype(a)> &&
                    std::is_integral_v<decltype(b)>)
        return b == 0 ? 0
                      : floor(static_cast<double>(a) / static_cast<double>(b));
      else
        return floor(a / b);
    },
    [](const units::Unit &a, const units::Unit &b) { return a / b; }};

// remainder defined as in Python
constexpr auto mod =
    overloaded{remainder_types_t{},
               [](const units::Unit &a, const units::Unit &b) { return a % b; },
               [](const auto a, const auto b) {
                 if constexpr (std::is_floating_point_v<decltype(a)> ||
                               std::is_floating_point_v<decltype(b)>) {
                   return b == 0 ? NAN : a - floor_divide(a, b) * b;
                 } else {
                   return b == 0 ? 0 : a - floor_divide(a, b) * b;
                 }
               }};

constexpr auto mod_equals = overloaded{
    arg_list<int64_t, int32_t, std::tuple<int64_t, int32_t>>,
    [](units::Unit &a, const units::Unit &b) { a %= b; },
    [](auto &&a, const auto &b) { a = mod(a, b); }};

constexpr auto unary_minus =
    overloaded{arg_list<double, float, int64_t, int32_t, Eigen::Vector3d>,
               [](const auto x) { return -x; }};

} // namespace scipp::core::element
