// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/inv.h"

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/core/transform_common.h"

#include "scipp/variable/transform.h"

namespace scipp::variable {
namespace element {
constexpr auto inv =
    overloaded{core::element::arg_list<Eigen::Matrix3d, Eigen::Affine3d,
                                       core::Translation, core::Quaternion>,
               core::transform_flags::expect_no_variance_arg<0>,
               core::transform_flags::expect_no_variance_arg<1>,
               [](const auto &transform) { return transform.inverse(); },
               [](const sc_units::Unit &) {
                 // The resulting unit depends on the dtype;
                 // the calling code assigns it.
                 return sc_units::none;
               }};
} // namespace element

namespace {
bool is_transform_with_translation(const Variable &var) {
  const auto dt = var.dtype();
  return dt == dtype<Eigen::Affine3d> || dt == dtype<scipp::core::Translation>;
}

// Translations: The unit stays the same because translations are additive.
// Affine transforms: The unit applies only to the translation part, see above.
// Linear transforms: Can scale the input, the unit is multiplicative.
// Rotations: A unit is ill-defined, but use 1/u to cancel out any unit
//            in case the user sets one manually.
auto result_unit(const Variable &var) {
  return is_transform_with_translation(var) ? var.unit()
                                            : sc_units::one / var.unit();
}

} // namespace

Variable inv(const Variable &var) {
  auto res = variable::transform(var, element::inv, "inverse");
  res.setUnit(result_unit(var));
  return res;
}
} // namespace scipp::variable
