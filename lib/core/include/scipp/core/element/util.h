// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cstddef>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {

/// Set the elements referenced by a span to 0
template <class T> void zero(const std::span<T> &data) {
  for (auto &x : data)
    x = 0.0;
}

/// Set the elements references by the spans for values and variances to 0
template <class T> void zero(const core::ValueAndVariance<std::span<T>> &data) {
  zero(data.value);
  zero(data.variance);
}

constexpr auto values = overloaded{
    transform_flags::no_out_variance, transform_flags::force_variance_broadcast,
    core::element::arg_list<double, float>, [](const auto &x) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
        return x.value;
      else
        return x;
    }};

constexpr auto variances =
    overloaded{transform_flags::no_out_variance,
               core::element::arg_list<double, float>,
               transform_flags::expect_variance_arg<0>,
               transform_flags::force_variance_broadcast,
               [](const auto &x) {
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return x.variance;
                 else
                   return x; // unreachable but required for instantiation
               },
               [](const sc_units::Unit &u) { return u * u; }};

constexpr auto stddevs =
    overloaded{transform_flags::no_out_variance,
               core::element::arg_list<double, float>,
               transform_flags::expect_variance_arg<0>,
               transform_flags::force_variance_broadcast,
               [](const auto &x) {
                 using std::sqrt;
                 if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                   return sqrt(x.variance);
                 else
                   return sqrt(x); // unreachable but required for instantiation
               },
               [](const sc_units::Unit &u) { return u; }};

constexpr auto issorted_common = overloaded{
    core::element::arg_list<
        std::tuple<bool, double, double>, std::tuple<bool, float, float>,
        std::tuple<bool, int64_t, int64_t>, std::tuple<bool, int32_t, int32_t>,
        std::tuple<bool, std::string, std::string>,
        std::tuple<bool, time_point, time_point>>,
    transform_flags::expect_no_variance_arg<1>,
    [](sc_units::Unit &out, const sc_units::Unit &left,
       const sc_units::Unit &right) {
      core::expect::equals(left, right);
      out = sc_units::none;
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
    overloaded{arg_list<std::span<const double>, std::span<const float>,
                        std::span<const int64_t>, std::span<const int32_t>,
                        std::span<const time_point>>,
               transform_flags::expect_no_variance_arg<0>,
               [](const sc_units::Unit &) { return sc_units::none; },
               [](const auto &range) { return numeric::islinspace(range); }};

constexpr auto isarange =
    overloaded{arg_list<std::span<const int64_t>, std::span<const int32_t>>,
               transform_flags::expect_no_variance_arg<0>,
               [](const sc_units::Unit &) { return sc_units::none; },
               [](const auto &range) { return numeric::isarange(range); }};

constexpr auto zip =
    overloaded{arg_list<int64_t>, transform_flags::expect_no_variance_arg<0>,
               transform_flags::expect_no_variance_arg<1>,
               [](const sc_units::Unit &first, const sc_units::Unit &second) {
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
                                [](const sc_units::Unit &u) { return u; }};

constexpr auto fill =
    overloaded{arg_list<double, float, std::tuple<float, double>>,
               [](auto &x, const auto &value) { x = value; }};

constexpr auto fill_zeros =
    overloaded{arg_list<double, float, int64_t, int32_t, SubbinSizes>,
               [](sc_units::Unit &) {}, [](auto &x) { x = 0; }};

template <class... Ts>
constexpr arg_list_t<std::tuple<bool, Ts, Ts>...> where_arg_list{};

constexpr auto where = overloaded{
    where_arg_list<double, float, int64_t, int32_t, bool, core::time_point,
                   index_pair, std::string, Eigen::Vector3d, Eigen::Matrix3d,
                   Eigen::Affine3d, core::Quaternion, core::Translation>,
    transform_flags::force_variance_broadcast,
    [](const auto &condition, const auto &x, const auto &y) {
      return condition ? x : y;
    },
    [](const sc_units::Unit &condition, const sc_units::Unit &x,
       const sc_units::Unit &y) {
      expect::equals(sc_units::none, condition);
      expect::equals(x, y);
      return x;
    }};

} // namespace scipp::core::element
