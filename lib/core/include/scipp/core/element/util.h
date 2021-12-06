// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cstddef>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {

/// Set the elements referenced by a span to 0
template <class T> void zero(const scipp::span<T> &data) {
  for (auto &x : data)
    x = 0.0;
}

/// Set the elements references by the spans for values and variances to 0
template <class T>
void zero(const core::ValueAndVariance<scipp::span<T>> &data) {
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

constexpr auto stddevs = overloaded{
    transform_flags::no_out_variance, core::element::arg_list<double, float>,
    transform_flags::expect_variance_arg<0>,
    [](const auto &x) {
      using std::sqrt;
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
        return sqrt(x.variance);
      else
        return sqrt(x); // unreachable but required for instantiation
    },
    [](const units::Unit &u) { return u; }};

constexpr auto issorted_common = overloaded{
    core::element::arg_list<
        std::tuple<bool, double, double>, std::tuple<bool, float, float>,
        std::tuple<bool, int64_t, int64_t>, std::tuple<bool, int32_t, int32_t>,
        std::tuple<bool, std::string, std::string>,
        std::tuple<bool, time_point, time_point>>,
    transform_flags::expect_no_variance_arg<1>,
    [](units::Unit &out, const units::Unit &left, const units::Unit &right) {
      core::expect::equals(left, right);
      out = units::dimensionless;
    }};

constexpr auto issorted_nondescending = overloaded{
    issorted_common, [](bool &out, const auto &left, const auto &right) {
      out = out && (left <= right);
    }};

constexpr auto issorted_nonascending = overloaded{
    issorted_common, [](bool &out, const auto &left, const auto &right) {
      out = out && (left >= right);
    }};

constexpr auto islinspace =
    overloaded{arg_list<scipp::span<const double>, scipp::span<const float>,
                        scipp::span<const int64_t>, scipp::span<const int32_t>,
                        scipp::span<const time_point>>,
               transform_flags::expect_no_variance_arg<0>,
               [](const units::Unit &) { return units::one; },
               [](const auto &range) { return numeric::islinspace(range); }};

constexpr auto zip =
    overloaded{arg_list<int64_t>, transform_flags::expect_no_variance_arg<0>,
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

constexpr auto where = overloaded{
    core::element::arg_list<
        std::tuple<bool, double, double>, std::tuple<bool, float, float>,
        std::tuple<bool, int64_t, int64_t>, std::tuple<bool, int32_t, int32_t>,
        std::tuple<bool, bool, bool>, std::tuple<bool, time_point, time_point>,
        std::tuple<bool, scipp::index_pair, scipp::index_pair>>,
    [](const auto &condition, const auto &x, const auto &y) {
      return condition ? x : y;
    },
    [](const units::Unit &condition, const units::Unit &x,
       const units::Unit &y) {
      expect::equals(condition, units::one);
      expect::equals(x, y);
      return x;
    }};

} // namespace scipp::core::element
