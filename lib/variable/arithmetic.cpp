// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/arithmetic.h"
#include "scipp/core/dtype.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arithmetic.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/pow.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

namespace {

bool is_transform_with_translation(const Variable &var) {
  return var.dtype() == dtype<Eigen::Affine3d> ||
         var.dtype() == dtype<scipp::core::Translation>;
}

auto make_factor(const Variable &prototype, const double value) {
  const auto unit = variableFactory().elem_unit(prototype) == sc_units::none
                        ? sc_units::none
                        : sc_units::one;
  return astype(makeVariable<double>(Values{value}, unit),
                variableFactory().elem_dtype(prototype));
}

/// True if a and b are correlated, currently only if referencing same.
bool correlated(const Variable &a, const Variable &b) {
  return variableFactory().has_variances(a) &&
         variableFactory().has_variances(b) && a.is_same(b);
}

} // namespace

Variable operator+(const Variable &a, const Variable &b) {
  if (correlated(a, b))
    return a * make_factor(a, 2.0);
  return transform(a, b, core::element::add, "add");
}

Variable operator-(const Variable &a, const Variable &b) {
  if (correlated(a, b))
    return a * make_factor(a, 0.0);
  return transform(a, b, core::element::subtract, "subtract");
}

Variable operator*(const Variable &a, const Variable &b) {
  if (is_transform_with_translation(a) &&
      (is_transform_with_translation(b) ||
       b.dtype() == dtype<Eigen::Vector3d>)) {
    return transform(a, b, core::element::apply_spatial_transformation,
                     std::string_view("apply_spatial_transformation"));
  } else {
    if (correlated(a, b))
      return pow(a, make_factor(a, 2.0));
    return transform(a, b, core::element::multiply,
                     std::string_view("multiply"));
  }
}

Variable operator/(const Variable &a, const Variable &b) {
  if (correlated(a, b))
    return pow(a, make_factor(a, 0.0));
  return transform(a, b, core::element::divide, "divide");
}

Variable &operator+=(Variable &a, const Variable &b) {
  operator+=(Variable(a), b);
  return a;
}

Variable &operator-=(Variable &a, const Variable &b) {
  operator-=(Variable(a), b);
  return a;
}

Variable &operator*=(Variable &a, const Variable &b) {
  operator*=(Variable(a), b);
  return a;
}

Variable &operator/=(Variable &a, const Variable &b) {
  operator/=(Variable(a), b);
  return a;
}

Variable &floor_divide_equals(Variable &a, const Variable &b) {
  floor_divide_equals(Variable(a), b);
  return a;
}

Variable operator+=(Variable &&a, const Variable &b) {
  if (correlated(a, b))
    return a *= make_factor(a, 2.0);
  transform_in_place(a, b, core::element::add_equals,
                     std::string_view("add_equals"));
  return std::move(a);
}

Variable operator-=(Variable &&a, const Variable &b) {
  if (correlated(a, b))
    return a *= make_factor(a, 0.0);
  transform_in_place(a, b, core::element::subtract_equals,
                     std::string_view("subtract_equals"));
  return std::move(a);
}

Variable operator*=(Variable &&a, const Variable &b) {
  if (correlated(a, b))
    return pow(a, make_factor(a, 2.0), a);
  transform_in_place(a, b, core::element::multiply_equals,
                     std::string_view("multiply_equals"));
  return std::move(a);
}

Variable operator/=(Variable &&a, const Variable &b) {
  if (correlated(a, b))
    return pow(a, make_factor(a, 0.0), a);
  transform_in_place(a, b, core::element::divide_equals,
                     std::string_view("divide_equals"));
  return std::move(a);
}

Variable floor_divide_equals(Variable &&a, const Variable &b) {
  transform_in_place(a, b, core::element::floor_divide_equals,
                     std::string_view("divide_equals"));
  return std::move(a);
}

} // namespace scipp::variable
