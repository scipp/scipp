// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#pragma once

#include <cmath>
#include <numbers>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {
namespace detail {
inline double deg_to_rad(const double x) {
  return x * std::numbers::pi_v<double> / 180.0;
}

inline float deg_to_rad(const float x) {
  return x * std::numbers::pi_v<float> / 180.0f;
}

inline ValueAndVariance<double> deg_to_rad(const ValueAndVariance<double> x) {
  return x * std::numbers::pi_v<double> / 180.0;
}

inline ValueAndVariance<float> deg_to_rad(const ValueAndVariance<float> x) {
  return x * std::numbers::pi_v<float> / 180.0f;
}
} // namespace detail

constexpr auto trig = overloaded{arg_list<double, float>};

constexpr auto sin = overloaded{
    trig,
    [](const auto &x) {
      using std::sin;
      return sin(x);
    },
};

constexpr auto sin_deg =
    overloaded{trig,
               [](const auto &x) {
                 using std::sin;
                 return sin(detail::deg_to_rad(x));
               },
               [](const sc_units::Unit &x) { return sc_units::sin(x); }};

constexpr auto cos = overloaded{trig, [](const auto &x) {
                                  using std::cos;
                                  return cos(x);
                                }};

constexpr auto cos_deg =
    overloaded{trig,
               [](const auto &x) {
                 using std::cos;
                 return cos(detail::deg_to_rad(x));
               },
               [](const sc_units::Unit &x) { return sc_units::cos(x); }};

constexpr auto tan = overloaded{trig, [](const auto &x) {
                                  using std::tan;
                                  return tan(x);
                                }};

constexpr auto tan_deg =
    overloaded{trig,
               [](const auto &x) {
                 using std::tan;
                 return tan(detail::deg_to_rad(x));
               },
               [](const sc_units::Unit &x) { return sc_units::tan(x); }};

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

constexpr auto atan2 = overloaded{
    trig, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<2>, [](const auto y, const auto x) {
      using std::atan2;
      return atan2(y, x);
    }};

} // namespace scipp::core::element
