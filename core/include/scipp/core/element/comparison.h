// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include <cmath>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

/// Operators to be used with transform and transform_in_place to implement
/// operations for Variable.
namespace scipp::core::element {

constexpr auto comparison = overloaded{
    transform_flags::no_out_variance,
    arg_list<double, float, int64_t, int32_t, std::tuple<int64_t, int32_t>,
             std::tuple<int32_t, int64_t>>,
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
    overloaded{arg_list<double, float, int64_t, int32_t>,
               transform_flags::expect_in_variance_if_out_variance,
               [](auto &&a, const auto &b) {
                 using std::max;
                 a = max(a, b);
               }};

constexpr auto min_equals =
    overloaded{arg_list<double, float, int64_t, int32_t>,
               transform_flags::expect_in_variance_if_out_variance,
               [](auto &&a, const auto &b) {
                 using std::min;
                 a = min(a, b);
               }};

} // namespace scipp::core::element
