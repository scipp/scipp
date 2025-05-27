// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#pragma once

#include <tuple>
#include <variant>

#include "pybind11.h"

#include "scipp/core/dtype.h"
#include "scipp/core/time_point.h"
#include "scipp/units/unit.h"

struct DefaultUnit {};

using ProtoUnit = std::variant<std::string, scipp::sc_units::Unit,
                               pybind11::none, DefaultUnit>;

std::tuple<scipp::sc_units::Unit, int64_t>
get_time_unit(std::optional<scipp::sc_units::Unit> value_unit,
              std::optional<scipp::sc_units::Unit> dtype_unit,
              scipp::sc_units::Unit sc_unit);

std::tuple<scipp::sc_units::Unit, int64_t>
get_time_unit(const pybind11::buffer &value, const pybind11::object &dtype,
              scipp::sc_units::Unit unit);

template <class T>
std::tuple<scipp::sc_units::Unit, scipp::sc_units::Unit>
common_unit(const pybind11::object &, const scipp::sc_units::Unit unit) {
  // In the general case, values and variances do not encode units themselves.
  return std::tuple{unit, unit};
}

template <>
std::tuple<scipp::sc_units::Unit, scipp::sc_units::Unit>
common_unit<scipp::core::time_point>(const pybind11::object &values,
                                     const scipp::sc_units::Unit unit);

/// Format a time unit as an ASCII string.
/// Only time units are supported!
// TODO Can be removed if / when the units library supports this.
std::string to_numpy_time_string(scipp::sc_units::Unit unit);
std::string to_numpy_time_string(const ProtoUnit &unit);

scipp::sc_units::Unit
unit_or_default(const ProtoUnit &unit,
                const scipp::core::DType type = scipp::core::dtype<void>);
