// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/creation.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

Variable empty(const Dimensions &dims, const units::Unit &unit,
               const DType type, const bool variances) {
  return variableFactory().create(type, dims, unit, variances);
}

Variable ones(const Dimensions &dims, const units::Unit &unit, const DType type,
              const bool variances) {
  const auto prototype =
      variances ? Variable{type, Dimensions{}, unit, Values{1}, Variances{1}}
                : Variable{type, Dimensions{}, unit, Values{1}};
  return broadcast(prototype, dims);
}

/// Create empty (uninitialized) variable with same parameters as prototype.
///
/// If specified, `shape` defines the shape of the output. If `prototype`
/// contains binned data `shape` may not be specified, instead `sizes` defines
/// the sizes of the desired bins.
Variable empty_like(const Variable &prototype,
                    const std::optional<Dimensions> &shape,
                    const Variable &sizes) {
  return variableFactory().empty_like(prototype, shape, sizes);
}

Variable special_like(const Variable &prototype, const FillValue &fill) {
  if (fill == FillValue::ZeroNotBool)
    return transform(prototype, core::element::zeros_not_bool_like);
  if (fill == FillValue::True)
    return transform(prototype, core::element::values_like<bool, true>);
  if (fill == FillValue::False)
    return transform(prototype, core::element::values_like<bool, false>);
  if (fill == FillValue::Max)
    return transform(prototype, core::element::numeric_limits_max_like);
  if (fill == FillValue::Lowest)
    return transform(prototype, core::element::numeric_limits_lowest_like);
  throw std::runtime_error("Unsupported fill value.");
}

} // namespace scipp::variable
