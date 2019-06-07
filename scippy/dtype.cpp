// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "bind_enum.h"
#include "except.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

void init_dtype(py::module &m) { bind_enum(m, "dtype", DType::Unknown); }
