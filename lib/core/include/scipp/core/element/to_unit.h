// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>
#include <limits>

#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

namespace {

// Source: https://stackoverflow.com/a/26584177/1458281
template <class T, class U> T safe_cast(const U x) {
  if (std::isnan(x))
    return std::numeric_limits<T>::min(); // behavior as numpy
  int exp;
  std::frexp(x, &exp);
  if (std::isfinite(x) && exp <= 8 * static_cast<int>(sizeof(T)) - 1)
    return x;
  return std::signbit(x) ? std::numeric_limits<T>::min()
                         : std::numeric_limits<T>::max();
}

template <class T>
constexpr auto round = [](const auto x) {
  if constexpr (std::is_integral_v<T>)
    return safe_cast<T>(x < 0 ? x - 0.5 : x + 0.5);
  else
    return static_cast<T>(x);
};
} // namespace

constexpr auto to_unit = overloaded{
    arg_list<double, std::tuple<float, double>, std::tuple<int64_t, double>,
             std::tuple<int32_t, double>, std::tuple<time_point, double>,
             std::tuple<Eigen::Vector3d, double>>,
    transform_flags::expect_no_variance_arg<1>,
    [](const units::Unit &, const units::Unit &target) { return target; },
    [](const auto &x, const auto &scale) {
      using T = std::decay_t<decltype(x)>;
      if constexpr (std::is_same_v<T, time_point>)
        return T{round<int64_t>(x.time_since_epoch() * scale)};
      else
        return round<T>(x * scale);
    }};

} // namespace scipp::core::element
