// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

/// Operators to be used with transform and transform_in_place to implement
/// geometric operations for Variable.
namespace scipp::core::element::geometry {

constexpr auto position = overloaded{
    arg_list<double>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<2>,
    [](const auto &x, const auto &y, const auto &z) {
      using T = double; // currently only double precision support
      return Eigen::Matrix<T, 3, 1>(x, y, z);
    },
    [](const units::Unit &x, const units::Unit &y, const units::Unit &z) {
      expect::equals(x, y);
      expect::equals(x, z);
      return x;
    }};

namespace detail {
template <int N>
static constexpr auto component =
    overloaded{arg_list<Eigen::Vector3d>,
               [](const auto &pos) { return pos[N]; },
               [](const units::Unit &u) { return u; }};
} // namespace detail
constexpr auto x = detail::component<0>;
constexpr auto y = detail::component<1>;
constexpr auto z = detail::component<2>;

} // namespace scipp::core::element::geometry
