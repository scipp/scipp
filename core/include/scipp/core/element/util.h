// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/common/span.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

#include <stddef.h>

namespace scipp::core::element {

/// Sets any masked elements to 0 to handle special FP vals
constexpr auto convertMaskedToZero = overloaded{
    core::element::arg_list<std::tuple<double, bool>, std::tuple<float, bool>,
                            std::tuple<bool, bool>, std::tuple<int64_t, bool>,
                            std::tuple<int32_t, bool>>,
    [](const auto &a, bool isMasked) { return isMasked ? decltype(a){0} : a; },
    [](const scipp::units::Unit &a, const scipp::units::Unit &b) {
      if (b != scipp::units::dimensionless) {
        throw except::UnitError("Expected mask to contain dimensionless units");
      }

      return a;
    }};

/// Set the elements referenced by a span to 0
template <class T> void zero(const scipp::span<T> &data) {
  for (auto &x : data)
    x = 0.0;
}

/// Set the elements references by the spans for values and variances to 0
template <class T> void zero(const core::ValueAndVariance<span<T>> &data) {
  zero(data.value);
  zero(data.variance);
}

constexpr auto values =
    overloaded{transform_flags::no_out_variance,
               core::element::arg_list<double, float>, [](const auto &x) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return x.value;
                 else
                   return x;
               }};

constexpr auto variances = overloaded{
    transform_flags::no_out_variance, core::element::arg_list<double, float>,
    transform_flags::expect_variance_arg<0>,
    [](const auto &x) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
        return x.variance;
      else
        return x; // unreachable but required for instantiation
    },
    [](const units::Unit &u) { return u * u; }};

} // namespace scipp::core::element
