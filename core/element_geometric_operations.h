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

constexpr auto zip_position = overloaded{
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

} // namespace element

} // namespace scipp::core

#endif // SCIPP_CORE_ELEMENT_GEOMETRIC_OPERATIONS_H
