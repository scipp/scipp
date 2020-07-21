// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <Eigen/Dense>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

constexpr auto add_inplace_types =
    arg_list<double, float, int64_t, int32_t, Eigen::Vector3d,
             std::tuple<scipp::core::time_point, int64_t>,
             std::tuple<scipp::core::time_point, int32_t>,
             std::tuple<double, float>, std::tuple<int64_t, int32_t>,
             std::tuple<int64_t, bool>>;

constexpr auto plus_equals = overloaded{
    add_inplace_types,
    [](auto &&a, const auto &b) { a += b; },
    [](scipp::core::time_point &a, const auto &b) {
      a += std::chrono::duration<int64_t>(b);
    },
};

constexpr auto minus_equals = overloaded{
    add_inplace_types,
    [](auto &&a, const auto &b) { a -= b; },
    [](scipp::core::time_point &a, const auto &b) {
      a -= std::chrono::duration<int64_t>(b);
    },
};

constexpr auto mul_inplace_types =
    arg_list<double, float, int64_t, int32_t, std::tuple<double, float>,
             std::tuple<float, double>, std::tuple<int64_t, int32_t>,
             std::tuple<Eigen::Vector3d, double>>;

constexpr auto times_equals =
    overloaded{mul_inplace_types, [](auto &&a, const auto &b) { a *= b; }};
constexpr auto divide_equals =
    overloaded{mul_inplace_types, [](auto &&a, const auto &b) { a /= b; }};

template <class... Ts> struct add_types_t {
  constexpr void operator()() const noexcept;
  using types = arithmetic_and_matrix_type_pairs;
};

template <class... Ts> struct times_types_t {
  constexpr void operator()() const noexcept;
  using types = arithmetic_type_pairs_with_bool;
};

template <class... Ts> struct divide_types_t {
  constexpr void operator()() const noexcept;
  using types = arithmetic_type_pairs;
};

constexpr auto plus = overloaded{
    add_types_t{}, [](const auto a, const auto b) { return a + b; },
    [](const scipp::core::time_point &a, const scipp::core::time_point &b) {
      // a + b for time_point is undefined. Can't throw from here
      // so return false
      (void)a;
      (void)b;
      return false;
    }};
constexpr auto minus = overloaded{
    add_types_t{}, [](const auto a, const auto b) { return a - b; },
    [](const scipp::core::time_point &a, const scipp::core::time_point &b) {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(a - b)
          .count();
    }};
constexpr auto times = overloaded{
    times_types_t{}, [](const auto a, const auto b) { return a * b; }};
constexpr auto divide = overloaded{
    divide_types_t{}, [](const auto a, const auto b) { return a / b; }};

constexpr auto unary_minus =
    overloaded{arg_list<double, float, int64_t, int32_t, Eigen::Vector3d>,
               [](const auto x) { return -x; }};

} // namespace scipp::core::element
