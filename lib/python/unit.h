// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#pragma once

#include <tuple>
#include <variant>

#include "nanobind.h"

#include "scipp/core/dtype.h"
#include "scipp/core/time_point.h"
#include "scipp/units/unit.h"

struct DefaultUnit {};

// std::monostate stands for Python None (nanobind maps None to monostate).
using ProtoUnit = std::variant<std::string, scipp::sc_units::Unit,
                               std::monostate, DefaultUnit>;

// Let ProtoUnit-typed arguments accept None (mapped to std::monostate).
// nanobind only does this automatically for std::optional and monostate
// itself, not for variants containing monostate.
namespace nanobind::detail {
template <> struct has_arg_defaults<ProtoUnit> : std::true_type {};
} // namespace nanobind::detail

std::tuple<scipp::sc_units::Unit, int64_t>
get_time_unit(std::optional<scipp::sc_units::Unit> value_unit,
              std::optional<scipp::sc_units::Unit> dtype_unit,
              scipp::sc_units::Unit sc_unit);

std::tuple<scipp::sc_units::Unit, int64_t>
get_time_unit(const nanobind::object &value, const nanobind::object &dtype,
              scipp::sc_units::Unit unit);

template <class T>
std::tuple<scipp::sc_units::Unit, scipp::sc_units::Unit>
common_unit(const nanobind::object &, const scipp::sc_units::Unit unit) {
  // In the general case, values and variances do not encode units themselves.
  return std::tuple{unit, unit};
}

template <>
std::tuple<scipp::sc_units::Unit, scipp::sc_units::Unit>
common_unit<scipp::core::time_point>(const nanobind::object &values,
                                     const scipp::sc_units::Unit unit);

/// Format a time unit as an ASCII string.
/// Only time units are supported!
// TODO Can be removed if / when the units library supports this.
std::string to_numpy_time_string(scipp::sc_units::Unit unit);
std::string to_numpy_time_string(const ProtoUnit &unit);

scipp::sc_units::Unit
unit_or_default(const ProtoUnit &unit,
                const scipp::core::DType type = scipp::core::dtype<void>);
