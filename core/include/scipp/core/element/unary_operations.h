// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

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
      using std::isnan;
      x = isnan(y) ? repl : y;
    }};

constexpr auto positive_inf_to_num =
    overloaded{replace_special, [](const auto &x, const auto &repl) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return isinf(x) && x.value > 0 ? repl : x;
                 else
                   return std::isinf(x) && x > 0 ? repl : x;
               }};

constexpr auto positive_inf_to_num_out_arg = overloaded{
    replace_special_out_arg, [](auto &x, const auto &y, const auto &repl) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(y)>>)
        x = isinf(y) && y.value > 0 ? repl : y;
      else
        x = std::isinf(y) && y > 0 ? repl : y;
    }};

constexpr auto negative_inf_to_num =
    overloaded{replace_special, [](const auto &x, const auto &repl) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return isinf(x) && x.value < 0 ? repl : x;
                 else
                   return std::isinf(x) && x < 0 ? repl : x;
               }};

constexpr auto negative_inf_to_num_out_arg = overloaded{
    replace_special_out_arg, [](auto &x, const auto &y, const auto &repl) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(y)>>)
        x = isinf(y) && y.value < 0 ? repl : y;
      else
        x = std::isinf(y) && y < 0 ? repl : y;
    }};

} // namespace scipp::core::element
