// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#pragma once

#include "pybind11.h"

#include "scipp/units/unit.h"

std::tuple<scipp::units::Unit, int64_t>
get_time_unit(std::optional<scipp::units::Unit> value_unit,
              std::optional<scipp::units::Unit> dtype_unit,
              scipp::units::Unit sc_unit);

std::tuple<scipp::units::Unit, int64_t>
get_time_unit(const pybind11::buffer &value, const pybind11::object &dtype,
              scipp::units::Unit unit);

/// Format a time unit as an ASCII string.
/// Only time units are supported!
// TODO Can be removed if / when the units library supports this.
std::string to_string_ascii_time(scipp::units::Unit unit);