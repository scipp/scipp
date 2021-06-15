// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include <cmath>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/values_and_variances.h"

/// Operators to be used with transform and transform_in_place to implement
/// operations for Variable.
namespace scipp::core::element {

namespace detail {

// Helper for making tuple out args Could be further generalised
template <class T> struct make_out_args {
  using type = std::tuple<bool, T, T, T>;
};

// Specialized helper for making output args tuple from input args tuple
template <class... T> struct make_out_args<std::tuple<T...>> {
  using type = std::tuple<bool, T...>;
};

// Convert tuple of input arg tuples to tuple of output args tuples
template <class... Ts> constexpr auto as_out_args(std::tuple<Ts...>) {
  return std::tuple<typename make_out_args<Ts>::type...>{};
}

} // namespace detail

using isclose_types_t = arg_list_t<
    double, float, int64_t, int32_t, std::tuple<float, float, double>,
    std::tuple<int64_t, int64_t, double>, std::tuple<int32_t, int32_t, double>,
    std::tuple<int32_t, int64_t, double>, std::tuple<int64_t, int32_t, double>,
    std::tuple<int64_t, int32_t, int64_t>,
    std::tuple<int32_t, int32_t, int64_t>,
    std::tuple<int32_t, int64_t, int64_t>>;

struct isclose_types_out_t {
  constexpr void operator()() const noexcept;
  using types = decltype(detail::as_out_args(isclose_types_t::types{}));
};

constexpr auto isclose_units = [](const units::Unit &x, const units::Unit &y,
                                  const units::Unit &t) {
  expect::equals(x, y);
  expect::equals(x, t);
  return units::dimensionless;
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

constexpr auto isclose_out =
    overloaded{transform_flags::expect_no_variance_arg_t<3>{}, isclose_units,
               [](auto &&out, const auto &x, const auto &y, const auto &t) {
                 out &= isclose(x, y, t);
               },
               isclose_types_out_t{}};

constexpr auto isclose_equal_nan_out =
    overloaded{transform_flags::expect_no_variance_arg_t<3>{},
               [](auto &&out, const auto &a, const auto &b, const auto tol) {
                 out &= isclose_equal_nan(a, b, tol);
               },
               isclose_types_out_t{}};

struct comparison_types_t {
  constexpr void operator()() const noexcept;
  using types = decltype(std::tuple_cat(std::declval<arithmetic_type_pairs>(),
                                        std::tuple<bool>{},
                                        std::tuple<core::time_point>{}));
};

constexpr auto comparison =
    overloaded{comparison_types_t{}, transform_flags::no_out_variance,
               [](const units::Unit &x, const units::Unit &y) {
                 expect::equals(x, y);
                 return units::dimensionless;
               }};

constexpr auto less = overloaded{
    comparison,
    [](const auto &x, const auto &y) { return x < y; },
};

constexpr auto greater = overloaded{
    comparison,
    [](const auto &x, const auto &y) { return x > y; },
};

constexpr auto less_equal = overloaded{
    comparison,
    [](const auto &x, const auto &y) { return x <= y; },
};

constexpr auto greater_equal =
    overloaded{comparison, [](const auto &x, const auto &y) { return x >= y; }};

constexpr auto equal = overloaded{
    comparison,
    [](const auto &x, const auto &y) { return x == y; },
};
constexpr auto not_equal =
    overloaded{comparison, [](const auto &x, const auto &y) { return x != y; }};

constexpr auto max_equals =
    overloaded{arg_list<double, float, int64_t, int32_t, bool, time_point>,
               transform_flags::expect_in_variance_if_out_variance,
               [](auto &&a, const auto &b) {
                 using std::max;
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
                 using std::min;
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
