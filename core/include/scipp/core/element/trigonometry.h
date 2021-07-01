// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#pragma once

#include <cmath>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {
constexpr auto trig = overloaded{arg_list<double, float>,
                                 transform_flags::expect_no_variance_arg<0>,
                                 transform_flags::expect_no_variance_arg<1>,
                                 transform_flags::expect_no_variance_arg<2>};

constexpr auto sin = overloaded{trig, [](const auto &x) {
                                  using std::sin;
                                  return sin(x);
                                }};

constexpr auto cos = overloaded{trig, [](const auto &x) {
                                  using std::cos;
                                  return cos(x);
                                }};

constexpr auto tan = overloaded{trig, [](const auto &x) {
                                  using std::tan;
                                  return tan(x);
                                }};

constexpr auto asin = overloaded{trig, [](const auto x) {
                                   using std::asin;
                                   return asin(x);
                                 }};

constexpr auto acos = overloaded{trig, [](const auto x) {
                                   using std::acos;
                                   return acos(x);
                                 }};

constexpr auto atan = overloaded{trig, [](const auto x) {
                                   using std::atan;
                                   return atan(x);
                                 }};

constexpr auto atan2 = overloaded{trig, [](const auto y, const auto x) {
                                    using std::atan2;
                                    return atan2(y, x);
                                  }};

} // namespace scipp::core::element
