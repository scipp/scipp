// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/multiply.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element/arithmetic.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

bool is_transform_with_translation(const Variable &var) {
  return var.dtype() == dtype<Eigen::Affine3d> ||
         var.dtype() == dtype<scipp::core::eigen_translation_type>;
}

Variable operator*(const Variable &a, const Variable &b) {
  if (is_transform_with_translation(a) && (is_transform_with_translation(b) || b.dtype() == dtype<Eigen::Vector3d>)) {
    return transform(a, b, core::element::apply_spatial_transformation,
                     std::string_view("apply_spatial_transformation"));
  } else {
    return transform(a, b, core::element::multiply,
                     std::string_view("multiply"));
  }
}

} // namespace scipp::variable