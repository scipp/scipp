// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_ELEMENT_UNARY_OPERATIONS_H
#define SCIPP_CORE_ELEMENT_UNARY_OPERATIONS_H

#include <cmath>

namespace scipp::core {

/// Operators to be used with transform and transform_in_place to implement
/// operations for Variable.
namespace element {

constexpr auto sqrt = [](const auto x) noexcept {
  using std::sqrt;
  return sqrt(x);
};

constexpr auto sqrt_out_arg = [](auto &x, const auto y) noexcept {
  using std::sqrt;
  x = sqrt(y);
};

} // namespace element

} // namespace scipp::core

#endif // SCIPP_CORE_ELEMENT_UNARY_OPERATIONS_H
