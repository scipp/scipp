// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_ELEMENT_UNARY_OPERATIONS_H
#define SCIPP_CORE_ELEMENT_UNARY_OPERATIONS_H

#include <cmath>

namespace scipp::core {

namespace element {

constexpr auto sqrt = [](const auto x) {
  using std::sqrt;
  return sqrt(x);
};

} // namespace element

} // namespace scipp::core

#endif // SCIPP_CORE_ELEMENT_UNARY_OPERATIONS_H
