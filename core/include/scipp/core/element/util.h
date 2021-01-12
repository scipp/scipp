// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/common/span.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/subbin_sizes.h"
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

constexpr auto is_sorted_common = overloaded{
    core::element::arg_list<
        std::tuple<bool, double, double>, std::tuple<bool, float, float>,
        std::tuple<bool, int64_t, int64_t>, std::tuple<bool, int32_t, int32_t>,
        std::tuple<bool, std::string, std::string>>,
    transform_flags::expect_no_variance_arg<1>,
    [](units::Unit &out, const units::Unit &left, const units::Unit &right) {
      core::expect::equals(left, right);
      out = units::dimensionless;
    }};

constexpr auto is_sorted_nondescending = overloaded{
    is_sorted_common, [](bool &out, const auto &left, const auto &right) {
      out = out && (left <= right);
    }};

constexpr auto is_sorted_nonascending = overloaded{
    is_sorted_common, [](bool &out, const auto &left, const auto &right) {
      out = out && (left >= right);
    }};

constexpr auto is_linspace =
    overloaded{arg_list<span<const double>, span<const float>>,
               transform_flags::expect_no_variance_arg<0>,
               [](const units::Unit &) { return units::one; },
               [](const auto &range) { return numeric::is_linspace(range); }};

constexpr auto zip = overloaded{
    arg_list<int64_t, int32_t>, transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const units::Unit &first, const units::Unit &second) {
      expect::equals(first, second);
      return first;
    },
    [](const auto first, const auto second) {
      return std::pair{first, second};
    }};

template <int N>
constexpr auto get = overloaded{arg_list<std::pair<scipp::index, scipp::index>>,
                                transform_flags::expect_no_variance_arg<0>,
                                [](const auto &x) { return std::get<N>(x); },
                                [](const units::Unit &u) { return u; }};

constexpr auto fill =
    overloaded{arg_list<double, float, std::tuple<float, double>>,
               [](auto &x, const auto &value) { x = value; }};

constexpr auto fill_zeros =
    overloaded{arg_list<double, float, int64_t, int32_t, SubbinSizes>,
               [](units::Unit &) {}, [](auto &x) { x = 0; }};

} // namespace scipp::core::element
