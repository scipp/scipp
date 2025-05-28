// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"
#include <Eigen/Geometry>
#include <cmath>

#if __cplusplus > 201703L
#include <numeric>
namespace scipp::core::element::detail {
using midpoint = std::midpoint;
}
#else
namespace scipp::core::element::detail {
template <class T> constexpr auto midpoint(const T &a, const T &b) {
  if constexpr (std::is_integral_v<T>) {
    using U = std::make_unsigned_t<T>;
    int sign = 1;
    U m = a;
    U M = b;
    if (a > b) {
      sign = -1;
      m = b;
      M = a;
    }
    return a + sign * static_cast<T>(static_cast<U>(M - m) / 2);
  } else {
    constexpr auto lo = std::numeric_limits<T>::min() * 2;
    constexpr auto hi = std::numeric_limits<T>::max() / 2;
    if (std::abs(a) <= hi && std::abs(b) <= hi)
      return (a + b) / 2; // always correctly rounded
    if (std::abs(a) < lo) // not safe to halve a
      return a + b / 2;
    if (std::abs(b) < lo) // not safe to halve b
      return a / 2 + b;
    return a / 2 + b / 2; // otherwise correctly rounded
  }
}
} // namespace scipp::core::element::detail
#endif

namespace scipp::core::element {

constexpr auto abs =
    overloaded{arg_list<double, float, int64_t, int32_t>, [](const auto x) {
                 using std::abs;
                 return abs(x);
               }};

constexpr auto norm = overloaded{arg_list<Eigen::Vector3d>,
                                 [](const auto &x) { return x.norm(); },
                                 [](const sc_units::Unit &x) { return x; }};

constexpr auto pow = overloaded{
    arg_list<std::tuple<double, double>, std::tuple<double, float>,
             std::tuple<double, int32_t>, std::tuple<double, int64_t>,
             std::tuple<float, double>, std::tuple<float, float>,
             std::tuple<float, int32_t>, std::tuple<float, int64_t>,
             std::tuple<int64_t, int64_t>, std::tuple<int64_t, int32_t>>,
    transform_flags::expect_no_variance_arg<1>, dimensionless_unit_check_return,
    [](const auto &base, const auto &exponent) {
      using numeric::pow;
      return pow(base, exponent);
    }};

constexpr auto pow_in_place = overloaded{
    arg_list<
        std::tuple<double, double, double>, std::tuple<double, double, float>,
        std::tuple<double, double, int32_t>,
        std::tuple<double, double, int64_t>, std::tuple<float, float, double>,
        std::tuple<float, float, float>, std::tuple<float, float, int32_t>,
        std::tuple<float, float, int64_t>,
        std::tuple<int64_t, int64_t, int64_t>,
        std::tuple<int64_t, int64_t, int32_t>>,
    transform_flags::expect_in_variance_if_out_variance,
    transform_flags::expect_no_variance_arg<2>,
    [](auto &out, const auto &base, const auto &exponent) {
      // Use element::pow instead of numeric::pow to inherit unit
      // handling.
      out = element::pow(base, exponent);
    }};

constexpr auto sqrt = overloaded{arg_list<double, float>, [](const auto x) {
                                   using std::sqrt;
                                   return sqrt(x);
                                 }};

constexpr auto dot = overloaded{
    arg_list<Eigen::Vector3d>,
    [](const auto &a, const auto &b) { return a.dot(b); },
    [](const sc_units::Unit &a, const sc_units::Unit &b) { return a * b; }};

constexpr auto cross = overloaded{
    arg_list<Eigen::Vector3d>,
    [](const auto &a, const auto &b) { return a.cross(b); },
    [](const sc_units::Unit &a, const sc_units::Unit &b) { return a * b; }};

constexpr auto reciprocal = overloaded{
    arg_list<double, float>,
    [](const auto &x) { return static_cast<std::decay_t<decltype(x)>>(1) / x; },
    [](const sc_units::Unit &unit) { return sc_units::one / unit; }};

constexpr auto exp =
    overloaded{arg_list<double, float>, dimensionless_unit_check_return,
               [](const auto &x) {
                 using std::exp;
                 return exp(x);
               }};

constexpr auto log =
    overloaded{arg_list<double, float>, dimensionless_unit_check_return,
               [](const auto &x) {
                 using std::log;
                 return log(x);
               }};

constexpr auto log10 =
    overloaded{arg_list<double, float>, dimensionless_unit_check_return,
               [](const auto &x) {
                 using std::log10;
                 return log10(x);
               }};

constexpr auto floor =
    overloaded{transform_flags::expect_no_variance_arg<0>,
               transform_flags::expect_no_variance_arg<1>,
               core::element::arg_list<double, float>, [](const auto &a) {
                 using std::floor;
                 return floor(a);
               }};

constexpr auto ceil =
    overloaded{transform_flags::expect_no_variance_arg<0>,
               transform_flags::expect_no_variance_arg<1>,
               core::element::arg_list<double, float>, [](const auto &a) {
                 using std::ceil;
                 return ceil(a);
               }};

constexpr auto rint =
    overloaded{transform_flags::expect_no_variance_arg<0>,
               transform_flags::expect_no_variance_arg<1>,
               core::element::arg_list<double, float>, [](const auto &a) {
                 using std::rint;
                 return rint(a);
               }};

constexpr auto special = overloaded{arg_list<double, float, int64_t, int32_t>,
                                    dimensionless_unit_check_return,
                                    transform_flags::expect_no_variance_arg<0>};

constexpr auto erf = overloaded{special, [](const auto &x) {
                                  using std::erf;
                                  return erf(x);
                                }};

constexpr auto erfc = overloaded{special, [](const auto &x) {
                                   using std::erfc;
                                   return erfc(x);
                                 }};

/*
 * Variances are not allowed because the outputs would be strongly correlated.
 * Given inputs (x, y, z), the midpoints have covariance
 *     Cov(mid(x, y), mid(y, z)) = Var(y) / 4
 * In the common case that all inputs have similar variances,
 * Pearson's correlation coefficient is
 *     rho ~ 1/2
 * that is, neighboring outputs are 50% correlated.
 */
constexpr auto midpoint = overloaded{
    arg_list<double, float, int64_t, int32_t, time_point>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const sc_units::Unit &a, const sc_units::Unit &b) {
      expect::equals(a, b);
      return a;
    },
    [](const auto &a, const auto &b) {
      if constexpr (std::is_same_v<std::decay_t<decltype(a)>, time_point>) {
        return time_point{
            detail::midpoint(a.time_since_epoch(), b.time_since_epoch())};
      } else {
        return detail::midpoint(a, b);
      }
    }};

} // namespace scipp::core::element
