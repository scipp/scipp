// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/creation.h"
#include "scipp/core/time_point.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

Variable empty(const Dimensions &dims, const sc_units::Unit &unit,
               const DType type, const bool with_variances,
               const bool aligned) {
  auto var = variableFactory().create(type, dims, unit, with_variances);
  var.set_aligned(aligned);
  return var;
}

Variable ones(const Dimensions &dims, const sc_units::Unit &unit,
              const DType type, const bool with_variances) {
  const auto make_prototype = [&](auto &&one) {
    return with_variances
               ? Variable{type, Dimensions{}, unit, Values{one}, Variances{one}}
               : Variable{type, Dimensions{}, unit, Values{one}};
  };
  if (type == dtype<core::time_point>) {
    return copy(broadcast(make_prototype(core::time_point{1}), dims));
  } else if (type == dtype<std::string>) {
    // This would result in a Variable containing (char)1.
    throw std::invalid_argument("Cannot construct 'ones' of strings.");
  } else {
    return copy(broadcast(make_prototype(1), dims));
  }
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

/// Create a variable with the same parameters as `prototype` with
/// values filled according to `fill`.
Variable special_like(const Variable &prototype, const FillValue &fill) {
  const char *name = "special_like";
  if (fill == FillValue::Default)
    return Variable(prototype, prototype.dims());
  if (fill == FillValue::ZeroNotBool)
    return transform(prototype, core::element::zeros_not_bool_like, name);
  if (fill == FillValue::True)
    return transform(prototype, core::element::values_like<bool, true>, name);
  if (fill == FillValue::False)
    return transform(prototype, core::element::values_like<bool, false>, name);
  if (fill == FillValue::Max)
    return transform(prototype, core::element::numeric_limits_max_like, name);
  if (fill == FillValue::Lowest)
    return transform(prototype, core::element::numeric_limits_lowest_like,
                     name);
  throw std::runtime_error("Unsupported fill value.");
}

/// Create a variable with the same parameters as `prototype` with the given
/// dimensions and values filled according to `fill`.
/// If `prototype` is binned, `accum` is dense
/// with the elem dtype of `prototype`.
Variable dense_special_like(const Variable &prototype,
                            const Dimensions &target_dims,
                            const FillValue &fill) {
  const auto type = variableFactory().elem_dtype(prototype);
  const auto unit = variableFactory().elem_unit(prototype);
  const auto has_variances = variableFactory().has_variances(prototype);
  const auto scalar_prototype = empty(Dimensions{}, unit, type, has_variances);
  return special_like(scalar_prototype.broadcast(target_dims), fill);
}

/// Create scalar variable containing 0 with same parameters as prototype.
Variable zero_like(const Variable &prototype) {
  return {prototype, Dimensions{}};
}

} // namespace scipp::variable
