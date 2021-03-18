// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

namespace {
template <class T>
constexpr auto round =
    [](const auto x) { return static_cast<T>(x < 0 ? x - 0.5 : x + 0.5); };
}

constexpr auto to_unit = overloaded{
    arg_list<double, std::tuple<float, double>, std::tuple<int64_t, double>,
             std::tuple<int32_t, double>, std::tuple<time_point, double>>,
    transform_flags::expect_no_variance_arg<1>,
    [](const units::Unit &, const units::Unit &target) { return target; },
    [](const auto &x, const auto &scale) {
      using T = std::decay_t<decltype(x)>;
      if constexpr (std::is_same_v<T, time_point>)
        return T{round<int64_t>(x.time_since_epoch() * scale)};
      else if constexpr (std::is_integral_v<T>)
        return round<T>(x * scale);
      else
        return static_cast<T>(x * scale);
    }};

} // namespace scipp::core::element
