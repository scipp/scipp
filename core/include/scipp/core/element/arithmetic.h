// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <Eigen/Dense>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"

namespace scipp::core::element {

constexpr auto add_types =
    arg_list<double, float, int64_t, int32_t, Eigen::Vector3d,
             std::tuple<double, float>, std::tuple<int64_t, int32_t>,
             std::tuple<int64_t, bool>>;

constexpr auto plus_equals =
    overloaded{add_types, [](auto &&a, const auto &b) { a += b; }};
constexpr auto minus_equals =
    overloaded{add_types, [](auto &&a, const auto &b) { a -= b; }};

constexpr auto mul_types =
    arg_list<double, float, int64_t, int32_t, std::tuple<double, float>,
             std::tuple<float, double>, std::tuple<int64_t, int32_t>,
             std::tuple<Eigen::Vector3d, double>>;

constexpr auto times_equals =
    overloaded{mul_types, [](auto &&a, const auto &b) { a *= b; }};
constexpr auto divide_equals =
    overloaded{mul_types, [](auto &&a, const auto &b) { a /= b; }};

} // namespace scipp::core::element
