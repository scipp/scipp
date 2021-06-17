// SPDX-License-Identifier: BSD-3-Clause
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

scipp::core::DType dtype_of(const pybind11::object &x);

scipp::core::DType cast_dtype(const pybind11::object &dtype);

scipp::core::DType scipp_dtype(const pybind11::object &type);

[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const std::string &dtype_name);
[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const pybind11::object &dtype);
