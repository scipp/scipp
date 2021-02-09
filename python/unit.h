// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#pragma once

#include "pybind11.h"

#include "scipp/units/unit.h"

std::tuple<scipp::units::Unit, int64_t, int64_t>
get_time_unit(std::optional<scipp::units::Unit> value_unit,
              std::optional<scipp::units::Unit> variance_unit,
              std::optional<scipp::units::Unit> dtype_unit,
              scipp::units::Unit sc_unit);

std::tuple<scipp::units::Unit, int64_t, int64_t>
get_time_unit(const pybind11::buffer &value,
              const std::optional<pybind11::buffer> &variance,
              const pybind11::object &dtype, scipp::units::Unit unit);