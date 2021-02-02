// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "pybind11.h"

#include "scipp/variable/trigonometry.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> void bind_atan2(py::module &m) {
  m.def(
      "atan2",
      [](const typename T::const_view_type &y,
         const typename T::const_view_type &x) { return atan2(y, x); },
      py::arg("y"), py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "atan2",
      [](const typename T::const_view_type &y,
         const typename T::const_view_type &x,
         typename T::view_type out) { return atan2(y, x, out); },
      py::arg("y"), py::arg("x"), py::arg("out"), py::keep_alive<0, 3>(),
      py::call_guard<py::gil_scoped_release>());
}

void init_trigonometry(py::module &m) { bind_atan2<Variable>(m); }
