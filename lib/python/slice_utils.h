// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "pybind11.h"

#include "scipp/variable/variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::variable;

std::tuple<Variable, Variable>
label_bounds_from_pyslice(const py::slice &py_slice);
