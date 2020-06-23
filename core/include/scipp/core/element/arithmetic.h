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
             std::tuple<double, float>, std::tuple<int64_t, int32_t>,
             std::tuple<int64_t, bool>>;

constexpr auto plus_equals =
    overloaded{add_inplace_types, [](auto &&a, const auto &b) { a += b; }};
constexpr auto minus_equals =
    overloaded{add_inplace_types, [](auto &&a, const auto &b) { a -= b; }};

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
constexpr auto divide = overloaded{
    divide_types_t{}, [](const auto a, const auto b) { return a / b; }};

constexpr auto unary_minus =
    overloaded{arg_list<double, float, int64_t, int32_t, Eigen::Vector3d>,
               [](const auto x) { return -x; }};

} // namespace scipp::core::element
