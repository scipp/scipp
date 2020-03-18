// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#ifndef SCIPP_CORE_ELEMENT_GEOMETRIC_OPERATIONS_H
#define SCIPP_CORE_ELEMENT_GEOMETRIC_OPERATIONS_H

#include "arg_list.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/transform.h"

namespace scipp::core {

/// Operators to be used with transform and transform_in_place to implement
/// geometric operations for Variable.

namespace element {

namespace geometry {

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
      expect::equals(x, units::m);
      return x;
    }};

namespace detail {
auto x_value = [](const auto &pos) { return pos[0]; };
auto y_value = [](const auto &pos) { return pos[1]; };
auto z_value = [](const auto &pos) { return pos[2]; };
auto unit_validate = [](const units::Unit &u) {
  expect::equals(u, units::m);
  return u;
};
} // namespace detail
constexpr auto x = overloaded{arg_list<Eigen::Vector3d>,
                              transform_flags::expect_no_variance_arg<0>,
                              detail::x_value, detail::unit_validate};
constexpr auto y = overloaded{arg_list<Eigen::Vector3d>,
                              transform_flags::expect_no_variance_arg<0>,
                              detail::y_value, detail::unit_validate};
constexpr auto z = overloaded{arg_list<Eigen::Vector3d>,
                              transform_flags::expect_no_variance_arg<0>,
                              detail::z_value, detail::unit_validate};

} // namespace geometry

} // namespace element

} // namespace scipp::core

#endif // SCIPP_CORE_ELEMENT_GEOMETRIC_OPERATIONS_H
