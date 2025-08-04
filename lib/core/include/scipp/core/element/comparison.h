// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include <cmath>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/values_and_variances.h"

/// Operators to be used with transform and transform_in_place to implement
/// operations for Variable.
namespace scipp::core::element {

using isclose_types_t = arg_list_t<
    double, float, int64_t, int32_t, std::tuple<float, float, double>,
    std::tuple<int64_t, int64_t, double>, std::tuple<int32_t, int32_t, double>,
    std::tuple<int32_t, int64_t, double>, std::tuple<int64_t, int32_t, double>,
    std::tuple<int64_t, int32_t, int64_t>,
    std::tuple<int32_t, int32_t, int64_t>,
    std::tuple<int32_t, int64_t, int64_t>>;

constexpr auto isclose_units = [](const sc_units::Unit &x,
                                  const sc_units::Unit &y,
                                  const sc_units::Unit &t) {
  expect::equals(x, y);
  expect::equals(x, t);
  return sc_units::none;
};

constexpr auto isclose = overloaded{
    transform_flags::expect_no_variance_arg_t<2>{}, isclose_types_t{},
    isclose_units, [](const auto &x, const auto &y, const auto &t) {
      using std::abs;
      return abs(x - y) <= t;
    }};

constexpr auto isclose_equal_nan = overloaded{
    transform_flags::expect_no_variance_arg_t<2>{}, isclose_types_t{},
    isclose_units, [](const auto &x, const auto &y, const auto &t) {
      using std::abs;
      using numeric::isnan;
      using numeric::isinf;
      using numeric::signbit;
      if (isnan(x) && isnan(y))
        return true;
      if (isinf(x) && isinf(y) && signbit(x) == signbit(y))
        return true;
      return abs(x - y) <= t;
    }};

struct comparison_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(std::tuple_cat(std::declval<arithmetic_type_pairs>(),
                                        std::tuple<bool>{},
                                        std::tuple<core::time_point>{}));
};

struct equality_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(std::tuple_cat(
      comparison_types_t::types{}, std::tuple<std::string>{},
      std::tuple<Eigen::Vector3d>{}, std::tuple<Eigen::Matrix3d>{},
      std::tuple<Eigen::Affine3d>{}, std::tuple<Quaternion>{},
      std::tuple<Translation>{}));
};

// Allow variance broadcasts because we just want to check for numeric equality.
// For inequalities, the variances are ignored anyway.
// See issue #3266
constexpr auto comparison = overloaded{
    transform_flags::no_out_variance, transform_flags::force_variance_broadcast,
    [](const sc_units::Unit &x, const sc_units::Unit &y) {
      expect::equals(x, y);
      return sc_units::none;
    }};

constexpr auto inequality = overloaded{comparison_types_t{}, comparison};

constexpr auto equality = overloaded{equality_types_t{}, comparison};

constexpr auto less = overloaded{
    inequality,
    [](const auto &x, const auto &y) { return x < y; },
};

constexpr auto greater = overloaded{
    inequality,
    [](const auto &x, const auto &y) { return x > y; },
};

constexpr auto less_equal = overloaded{
    inequality,
    [](const auto &x, const auto &y) { return x <= y; },
};

constexpr auto greater_equal =
    overloaded{inequality, [](const auto &x, const auto &y) { return x >= y; }};

constexpr auto equal = overloaded{
    equality,
    [](const auto &x, const auto &y) {
      // cppcheck-suppress constStatement
      using numeric::operator==;
      return x == y;
    },
};
constexpr auto not_equal =
    overloaded{equality, [](const auto &x, const auto &y) {
                 // cppcheck-suppress constStatement
                 using numeric::operator!=;
                 return x != y;
               }};

constexpr auto max_equals =
    overloaded{arg_list<double, float, int64_t, int32_t, bool, time_point>,
               transform_flags::expect_in_variance_if_out_variance,
               [](auto &&a, const auto &b) {
                 using numeric::isnan;
                 using std::max;
                 if (isnan(b))
                   a = b;
                 else if (!isnan(a))
                   a = max(a, b);
               }};

constexpr auto nanmax_equals =
    overloaded{arg_list<double, float, int64_t, int32_t, bool, time_point>,
               transform_flags::expect_in_variance_if_out_variance,
               [](auto &&a, const auto &b) {
                 using numeric::isnan;
                 using std::max;
                 if (isnan(a))
                   a = b;
                 if (!isnan(b))
                   a = max(a, b);
               }};

constexpr auto min_equals =
    overloaded{arg_list<double, float, int64_t, int32_t, bool, time_point>,
               transform_flags::expect_in_variance_if_out_variance,
               [](auto &&a, const auto &b) {
                 using numeric::isnan;
                 using std::min;
                 if (isnan(b))
                   a = b;
                 else if (!isnan(a))
                   a = min(a, b);
               }};

constexpr auto nanmin_equals =
    overloaded{arg_list<double, float, int64_t, int32_t, bool, time_point>,
               transform_flags::expect_in_variance_if_out_variance,
               [](auto &&a, const auto &b) {
                 using numeric::isnan;
                 using std::min;
                 if (isnan(a))
                   a = b;
                 if (!isnan(b))
                   a = min(a, b);
               }};

} // namespace scipp::core::element
