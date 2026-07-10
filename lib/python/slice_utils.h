// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "nanobind.h"

#include "scipp/variable/variable.h"

namespace nb = nanobind;
using namespace scipp;
using namespace scipp::variable;

std::tuple<Variable, Variable>
label_bounds_from_pyslice(const nb::slice &py_slice);
