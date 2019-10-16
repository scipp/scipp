// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#ifndef SCIPPY_DTYPE_PYTHON
#define SCIPPY_DTYPE_PYTHON

#include "pybind11.h"
#include <scipp/core/dtype.h>

namespace pybind11 {
class dtype;
}

scipp::core::DType scipp_dtype(const pybind11::object &type);

#endif // SCIPPY_DTYPE_PYTHON
