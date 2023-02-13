// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/inverse.h"

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/core/transform_common.h"

#include "scipp/variable/transform.h"

namespace scipp::variable {
namespace element {
constexpr auto inverse =
    overloaded{core::element::arg_list<Eigen::Matrix3d, Eigen::Affine3d,
                                       core::Translation, core::Quaternion>,
               core::transform_flags::expect_no_variance_arg<0>,
               core::transform_flags::expect_no_variance_arg<1>,
               [](const auto &transform) { return transform.inverse(); },
               [](const units::Unit &u) {
                 // This is not correct for linear transforms (Eigen::Matrix3d).
                 // But that case is handled separately by the caller.
                 return u;
               }};
}

Variable inverse(const Variable &var) {
  const auto result_unit = (var.dtype() == dtype<Eigen::Matrix3d>)
                               ? units::one / var.unit()
                               : var.unit();
  auto res = variable::transform(var, element::inverse, "inverse");
  res.setUnit(result_unit);
  return res;
}
} // namespace scipp::variable