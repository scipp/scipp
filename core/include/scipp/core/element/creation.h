// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <limits>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/transform_common.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {

constexpr auto special_like =
    overloaded{arg_list<double, float, int64_t, int32_t, bool, SubbinSizes>,
               [](const units::Unit &u) { return u; }};

constexpr auto zeros_not_bool_like =
    overloaded{special_like, [](const auto &x) {
                 using T = std::decay_t<decltype(x)>;
                 if constexpr (std::is_same_v<T, bool>)
                   return int64_t{0};
                 else
                   return T{0};
               }};

template <class T, T Value>
constexpr auto values_like =
    overloaded{special_like, [](const auto &) { return Value; }};

template <class T> struct underlying { using type = T; };
template <class T> struct underlying<ValueAndVariance<T>> { using type = T; };
template <class T> using underlying_t = typename underlying<T>::type;

constexpr auto numeric_limits_max_like =
    overloaded{special_like, [](const auto &x) {
                 return std::numeric_limits<
                     underlying_t<std::decay_t<decltype(x)>>>::max();
               }};

constexpr auto numeric_limits_lowest_like =
    overloaded{special_like, [](const auto &x) {
                 return std::numeric_limits<
                     underlying_t<std::decay_t<decltype(x)>>>::lowest();
               }};

} // namespace scipp::core::element
