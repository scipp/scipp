// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <limits>

#include "scipp/common/initialization.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/transform_common.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {

constexpr auto special_like =
    overloaded{arg_list<double, float, int64_t, int32_t, uint64_t, bool,
                        SubbinSizes, time_point, Eigen::Vector3d>,
               transform_flags::force_variance_broadcast,
               [](const sc_units::Unit &u) { return u; }};

/// Zero for accumulating a sum of the prototype's values.
///
/// bool and int32 accumulate in int64, which then also is the dtype of the
/// reduction's result: a sum of bools is a count, and sums of int32 leave the
/// int32 range after comparatively few elements, wrapping to a negative value
/// with no indication. int64 sums can still overflow, just as silently, but
/// there is no wider integer to promote to. Floating-point types keep their own
/// dtype: their range accommodates the sum, and only precision is at stake,
/// which `sum_into` handles by accumulating float32 in double.
constexpr auto zeros_for_sum_like = overloaded{
    special_like, []<class T>(const T &) {
      if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int32_t>)
        return int64_t{0};
      else
        return zero_init<T>::value();
    }};

template <class T, T Value>
constexpr auto values_like =
    overloaded{special_like, [](const auto &) { return Value; }};

template <class T> struct underlying {
  using type = T;
};
template <class T> struct underlying<ValueAndVariance<T>> {
  using type = T;
};
template <> struct underlying<time_point> {
  using type = decltype(std::declval<time_point>().time_since_epoch());
};
template <class T> using underlying_t = typename underlying<T>::type;

constexpr auto numeric_limits_max_like =
    overloaded{special_like, [](const auto &x) {
                 using T = std::decay_t<decltype(x)>;
                 return T{std::numeric_limits<underlying_t<T>>::max()};
               }};

constexpr auto numeric_limits_lowest_like =
    overloaded{special_like, [](const auto &x) {
                 using T = std::decay_t<decltype(x)>;
                 return T{std::numeric_limits<underlying_t<T>>::lowest()};
               }};

} // namespace scipp::core::element
