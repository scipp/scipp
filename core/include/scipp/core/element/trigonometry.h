// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#pragma once

#include <cmath>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core {

/// Operators to be used with transform and transform_in_place to implement
/// operations for Variable.
namespace element {

constexpr auto sin_out_arg = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](auto &x, const auto y) {
      using std::sin;
      x = sin(y);
    }};

constexpr auto cos_out_arg = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](auto &x, const auto y) {
      using std::cos;
      x = cos(y);
    }};

constexpr auto tan_out_arg = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](auto &x, const auto y) {
      using std::tan;
      x = tan(y);
    }};

constexpr auto asin =
    overloaded{transform_flags::expect_no_variance_arg<0>, [](const auto x) {
                 using std::asin;
                 return asin(x);
               }};

constexpr auto asin_out_arg = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](auto &x, const auto y) {
      using std::asin;
      x = asin(y);
    }};

constexpr auto acos =
    overloaded{transform_flags::expect_no_variance_arg<0>, [](const auto x) {
                 using std::acos;
                 return acos(x);
               }};

constexpr auto acos_out_arg = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](auto &x, const auto y) {
      using std::acos;
      x = acos(y);
    }};

constexpr auto atan =
    overloaded{transform_flags::expect_no_variance_arg<0>, [](const auto x) {
                 using std::atan;
                 return atan(x);
               }};

constexpr auto atan_out_arg = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](auto &x, const auto y) {
      using std::atan;
      x = atan(y);
    }};

constexpr auto atan2 = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](const auto y, const auto x) {
      using std::atan2;
      return atan2(y, x);
    }};

constexpr auto atan2_out_arg = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<2>,
    [](auto &out, const auto y, const auto x) {
      using std::atan2;
      out = atan2(y, x);
    }};

} // namespace element

} // namespace scipp::core
