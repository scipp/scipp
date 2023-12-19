// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#pragma once

#include <cmath>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {
constexpr auto hyperbolic = overloaded{
    arg_list<double, float>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>};

constexpr auto sinh = overloaded{hyperbolic, [](const auto &x) {
                                   using std::sinh;
                                   return sinh(x);
                                 }};

constexpr auto cosh = overloaded{hyperbolic, [](const auto &x) {
                                   using std::cosh;
                                   return cosh(x);
                                 }};

constexpr auto tanh = overloaded{hyperbolic, [](const auto &x) {
                                   using std::tanh;
                                   return tanh(x);
                                 }};

constexpr auto asinh = overloaded{hyperbolic, [](const auto x) {
                                    using std::asinh;
                                    return asinh(x);
                                  }};

constexpr auto acosh = overloaded{hyperbolic, [](const auto x) {
                                    using std::acosh;
                                    return acosh(x);
                                  }};

constexpr auto atanh = overloaded{hyperbolic, [](const auto x) {
                                    using std::atanh;
                                    return atanh(x);
                                  }};

} // namespace scipp::core::element
