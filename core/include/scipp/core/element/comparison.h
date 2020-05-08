// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include <cmath>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core {

/// Operators to be used with transform and transform_in_place to implement
/// operations for Variable.
namespace element {

constexpr auto comparison =
    overloaded{arg_list<double, float, int64_t, int32_t>,
               transform_flags::expect_no_variance_arg<0>,
               transform_flags::expect_no_variance_arg<1>,
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

struct max_equals
    : public transform_flags::expect_in_variance_if_out_variance_t {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const noexcept {
    using std::max;
    a = max(a, b);
  }
  using types = pair_self_t<double, float, int64_t, int32_t>;
};
struct min_equals
    : public transform_flags::expect_in_variance_if_out_variance_t {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const noexcept {
    using std::min;
    a = min(a, b);
  }
  using types = pair_self_t<double, float, int64_t, int32_t>;
};

} // namespace element

} // namespace scipp::core
