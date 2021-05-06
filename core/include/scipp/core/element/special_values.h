// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>
#include <type_traits>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

constexpr auto special_value_args = arg_list<int32_t, int64_t, double, float>;

constexpr auto special_value_args_finite =
    arg_list<int32_t, int64_t, double, float, Eigen::Vector3d>;

constexpr auto isnan =
    overloaded{special_value_args,
               [](const auto x) {
                 using numeric::isnan;
                 return isnan(x);
               },
               [](const units::Unit &) { return units::dimensionless; }};

constexpr auto isinf =
    overloaded{special_value_args,
               [](const auto x) {
                 using numeric::isinf;
                 return isinf(x);
               },
               [](const units::Unit &) { return units::dimensionless; }};

constexpr auto isfinite = overloaded{
    special_value_args_finite,
    [](const auto x) {
      if constexpr (std::is_same_v<std::decay_t<decltype(x)>,
                                   Eigen::Vector3d>) {
        return x.allFinite();
      } else {
        using numeric::isfinite;
        return isfinite(x);
      }
    },
    [](const units::Unit &) {
      return units::dimensionless;
    } // namespace scipp::core::element
};

namespace detail {
template <typename T> auto isposinf(T x) {
  return numeric::isinf(x) && !numeric::signbit(x);
}

template <typename T> auto isneginf(T x) {
  return numeric::isinf(x) && numeric::signbit(x);
}
} // namespace detail

constexpr auto isposinf =
    overloaded{special_value_args,
               [](const auto x) {
                 using detail::isposinf;
                 return isposinf(x);
               },
               [](const units::Unit &) { return units::dimensionless; }};

constexpr auto isneginf =
    overloaded{special_value_args,
               [](const auto x) {
                 using detail::isneginf;
                 return isneginf(x);
               },
               [](const units::Unit &) { return units::dimensionless; }};

constexpr auto replace_special = overloaded{
    arg_list<double, float>, transform_flags::expect_all_or_none_have_variance,
    [](const units::Unit &x, const units::Unit &repl) {
      expect::equals(x, repl);
      return x;
    }};

constexpr auto replace_special_out_arg = overloaded{
    arg_list<double, float>, transform_flags::expect_all_or_none_have_variance,
    [](units::Unit &a, const units::Unit &b, const units::Unit &repl) {
      expect::equals(b, repl);
      a = b;
    }};

constexpr auto nan_to_num =
    overloaded{replace_special, [](const auto x, const auto &repl) {
                 using std::isnan;
                 return isnan(x) ? repl : x;
               }};

constexpr auto nan_to_num_out_arg = overloaded{
    replace_special_out_arg, [](auto &x, const auto y, const auto &repl) {
      using numeric::isnan;
      x = isnan(y) ? repl : y;
    }};

constexpr auto positive_inf_to_num =
    overloaded{replace_special, [](const auto &x, const auto &repl) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return isinf(x) && x.value > 0 ? repl : x;
                 else
                   return numeric::isinf(x) && x > 0 ? repl : x;
               }};

constexpr auto positive_inf_to_num_out_arg = overloaded{
    replace_special_out_arg, [](auto &x, const auto &y, const auto &repl) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(y)>>)
        x = isinf(y) && y.value > 0 ? repl : y;
      else
        x = numeric::isinf(y) && y > 0 ? repl : y;
    }};

constexpr auto negative_inf_to_num =
    overloaded{replace_special, [](const auto &x, const auto &repl) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return isinf(x) && x.value < 0 ? repl : x;
                 else
                   return numeric::isinf(x) && x < 0 ? repl : x;
               }};

constexpr auto negative_inf_to_num_out_arg = overloaded{
    replace_special_out_arg, [](auto &x, const auto &y, const auto &repl) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(y)>>)
        x = isinf(y) && y.value < 0 ? repl : y;
      else
        x = numeric::isinf(y) && y < 0 ? repl : y;
    }};

} // namespace scipp::core::element
