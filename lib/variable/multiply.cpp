// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/multiply.h"
#include "scipp/core/element/arithmetic.h"
#include "scipp/core/dtype.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

Variable operator*(const Variable &a, const Variable &b) {
  if (a.dtype() == dtype<Eigen::Affine3d>) {
    return transform(a, b, core::element::multiply_if_units_equal,
                     std::string_view("multiply_if_units_equal"));
  } else {
    return transform(a, b, core::element::multiply,
                     std::string_view("multiply"));
  }
}

} // namespace scipp::variable