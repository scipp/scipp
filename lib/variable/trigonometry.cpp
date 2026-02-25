// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)

#include <format>
#include <string_view>

#include "scipp/core/element/trigonometry.h"
#include "scipp/core/flags.h"
#include "scipp/core/transform_common.h"
#include "scipp/units/except.h"
#include "scipp/units/string.h"
#include "scipp/units/unit.h"
#include "scipp/variable/to_unit.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/trigonometry.h"
#include "scipp/variable/variable.h"

using namespace scipp::core;

namespace scipp::variable {
namespace {
Variable trig(const Variable &var, auto op, auto op_deg,
              const std::string_view &name) {
  if (var.unit() == sc_units::deg) {
    return transform(var, op_deg, name);
  }

  // Rely on the element function to detect bad units. The following should only
  // succeed for rad and deg (which is filtered out above)
  return transform(var, op, name);
}
} // namespace

Variable sin(const Variable &var) {
  return trig(var, element::sin, element::sin_deg, "sin");
}

Variable &sin(const Variable &var, Variable &out) {
  transform_in_place(out, var, assign_unary{element::sin},
                     std::string_view("sin"));
  return out;
}

Variable cos(const Variable &var) {
  return trig(var, element::cos, element::cos_deg, "cos");
}

Variable &cos(const Variable &var, Variable &out) {
  transform_in_place(out, var, assign_unary{element::cos},
                     std::string_view("cos"));
  return out;
}

Variable tan(const Variable &var) {
  return trig(var, element::tan, element::tan_deg, "tan");
}

Variable &tan(const Variable &var, Variable &out) {
  transform_in_place(out, var, assign_unary{element::tan},
                     std::string_view("tan"));
  return out;
}
} // namespace scipp::variable
