// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/multiply.h"
#include "scipp/core/element/arithmetic.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

Variable operator*(const Variable &a, const Variable &b) {
  // branch here to different new kernel defined in core::element for
  // translation-dtypes.
  return transform(a, b, core::element::multiply, std::string_view("multiply"));
}

} // namespace scipp::variable
