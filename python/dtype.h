// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <string_view>

#include "pybind11.h"
#include <scipp/core/dtype.h>
#include <scipp/units/unit.h>

namespace pybind11 {
class dtype;
}

scipp::core::DType scipp_dtype(const pybind11::object &type);

[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(std::string_view dtype_name);
[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const pybind11::object &dtype);