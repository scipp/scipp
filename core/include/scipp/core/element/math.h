// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>

#include <Eigen/Dense>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

constexpr auto abs =
    overloaded{arg_list<double, float>, [](const auto x) noexcept {
                 using std::abs;
                 return abs(x);
               }};

constexpr auto norm = overloaded{arg_list<Eigen::Vector3d>,
                                 [](const auto &x) { return x.norm(); },
                                 [](const units::Unit &x) { return x; }};

constexpr auto sqrt =
    overloaded{arg_list<double, float>, [](const auto x) noexcept {
                 using std::sqrt;
                 return sqrt(x);
               }};

constexpr auto dot = overloaded{
    arg_list<Eigen::Vector3d>,
    [](const auto &a, const auto &b) { return a.dot(b); },
    [](const units::Unit &a, const units::Unit &b) { return a * b; }};

constexpr auto reciprocal = overloaded{
    arg_list<double, float>,
    [](const auto &x) { return static_cast<std::decay_t<decltype(x)>>(1) / x; },
    [](const units::Unit &unit) { return units::one / unit; }};

constexpr auto reciprocal_out_arg = overloaded{
    arg_list<double, float>,
    [](auto &x, const auto &y) {
      x = static_cast<std::decay_t<decltype(y)>>(1) / y;
    },
    [](units::Unit &x, const units::Unit &y) { x = units::one / y; }};

constexpr auto exp = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    dimensionless_unit_check_return, [](const auto &x) {
      using std::exp;
      return exp(x);
    }};

constexpr auto log = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    dimensionless_unit_check_return, [](const auto &x) {
      using std::log;
      return log(x);
    }};

constexpr auto log10 = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    dimensionless_unit_check_return, [](const auto &x) {
      using std::log10;
      return log10(x);
    }};

} // namespace scipp::core::element
