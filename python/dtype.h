// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include "pybind11.h"
#include <scipp/core/dtype.h>

namespace pybind11 {
class dtype;
}

scipp::core::DType scipp_dtype(const pybind11::object &type);
