// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "pybind11.h"

#include "scipp/variable/trigonometry.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> void bind_atan2(py::module &m) {
  m.def(
      "atan2", [](const T &y, const T &x) { return atan2(y, x); },
      py::kw_only(), py::arg("y"), py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "atan2", [](const T &y, const T &x, T &out) { return atan2(y, x, out); },
      py::kw_only(), py::arg("y"), py::arg("x"), py::arg("out"),
      py::call_guard<py::gil_scoped_release>());
}

void init_trigonometry(py::module &m) { bind_atan2<Variable>(m); }
