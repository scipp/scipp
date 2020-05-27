// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core {

/// Operators to be used with transform and transform_in_place to implement
/// operations for Variable.
namespace element {

auto unit_check_and_assign = [](units::Unit &a, const units::Unit &b,
                                const units::Unit &repl) {
  expect::equals(b, repl);
  a = b;
};

auto unit_check_and_return = [](const units::Unit &x, const units::Unit &repl) {
  expect::equals(x, repl);
  return x;
};

constexpr auto nan_to_num =
    overloaded{transform_flags::expect_all_or_none_have_variance,
               [](const auto x, const auto &repl) {
                 using std::isnan;
                 return isnan(x) ? repl : x;
               },
               unit_check_and_return};

constexpr auto nan_to_num_out_arg =
    overloaded{transform_flags::expect_all_or_none_have_variance,
               [](auto &x, const auto y, const auto &repl) {
                 using std::isnan;
                 x = isnan(y) ? repl : y;
               },
               unit_check_and_assign};

constexpr auto positive_inf_to_num =
    overloaded{transform_flags::expect_all_or_none_have_variance,
               [](const auto &x, const auto &repl) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return isinf(x) && x.value > 0 ? repl : x;
                 else
                   return std::isinf(x) && x > 0 ? repl : x;
               },
               unit_check_and_return};

constexpr auto positive_inf_to_num_out_arg =
    overloaded{transform_flags::expect_all_or_none_have_variance,
               [](auto &x, const auto &y, const auto &repl) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(y)>>)
                   x = isinf(y) && y.value > 0 ? repl : y;
                 else
                   x = std::isinf(y) && y > 0 ? repl : y;
               },
               unit_check_and_assign};

constexpr auto negative_inf_to_num =
    overloaded{transform_flags::expect_all_or_none_have_variance,
               [](const auto &x, const auto &repl) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return isinf(x) && x.value < 0 ? repl : x;
                 else
                   return std::isinf(x) && x < 0 ? repl : x;
               },
               unit_check_and_return};

constexpr auto negative_inf_to_num_out_arg =
    overloaded{transform_flags::expect_all_or_none_have_variance,
               [](auto &x, const auto &y, const auto &repl) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(y)>>)
                   x = isinf(y) && y.value < 0 ? repl : y;
                 else
                   x = std::isinf(y) && y < 0 ? repl : y;
               },
               unit_check_and_assign};

} // namespace element

} // namespace scipp::core
