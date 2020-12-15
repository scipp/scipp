// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <Eigen/Dense>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

constexpr auto add_inplace_types =
    arg_list<double, float, int64_t, int32_t, Eigen::Vector3d,
             std::tuple<scipp::core::time_point, int64_t>,
             std::tuple<scipp::core::time_point, int32_t>,
             std::tuple<double, float>, std::tuple<float, double>,
             std::tuple<int64_t, int32_t>, std::tuple<int32_t, int64_t>,
             std::tuple<double, int64_t>, std::tuple<double, int32_t>,
             std::tuple<float, int64_t>, std::tuple<float, int32_t>,
             std::tuple<int64_t, bool>>;

template <class T> struct ValueType { using value_type = T; };
template <class T> struct ValueType<ValueAndVariance<T>> {
  using value_type = T;
};

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

template <class... Ts> struct add_types_t {
  constexpr void operator()() const noexcept;
  using types = arithmetic_and_matrix_type_pairs;
};

template <class... Ts> struct times_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(std::tuple_cat(
      std::declval<arithmetic_type_pairs_with_bool>(),
      std::tuple<std::tuple<Eigen::Matrix3d, Eigen::Matrix3d>>(),
      std::tuple<std::tuple<Eigen::Matrix3d, Eigen::Vector3d>>()));
};

template <class... Ts> struct divide_types_t {
  constexpr void operator()() const noexcept;
  using types = arithmetic_type_pairs;
};

constexpr auto plus =
    overloaded{add_types_t{}, [](const auto a, const auto b) { return a + b; }};
constexpr auto minus =
    overloaded{add_types_t{}, [](const auto a, const auto b) { return a - b; }};
constexpr auto times = overloaded{
    times_types_t{}, [](const auto a, const auto b) { return a * b; }};
constexpr auto divide =
    overloaded{divide_types_t{}, [](const auto a, const auto b) {
                 // Integer division is truediv, as in Python 3 and numpy
                 if constexpr (std::is_integral_v<decltype(a)> &&
                               std::is_integral_v<decltype(b)>)
                   return static_cast<double>(a) / b;
                 else
                   return a / b;
               }};

constexpr auto unary_minus =
    overloaded{arg_list<double, float, int64_t, int32_t, Eigen::Vector3d>,
               [](const auto x) { return -x; }};

} // namespace scipp::core::element
