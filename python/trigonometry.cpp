// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "pybind11.h"

#include "scipp/variable/trigonometry.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> void bind_sin(py::module &m) {
  m.def(
      "sin", [](const typename T::const_view_type &x) { return sin(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "sin",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return sin(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_asin(py::module &m) {
  m.def(
      "asin", [](const typename T::const_view_type &x) { return asin(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "asin",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return asin(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_cos(py::module &m) {
  m.def(
      "cos", [](const typename T::const_view_type &x) { return cos(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "cos",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return cos(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_acos(py::module &m) {
  m.def(
      "acos", [](const typename T::const_view_type &x) { return acos(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "acos",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return acos(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_tan(py::module &m) {
  m.def(
      "tan", [](const typename T::const_view_type &x) { return tan(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "tan",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return tan(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_atan(py::module &m) {
  m.def(
      "atan", [](const typename T::const_view_type &x) { return atan(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "atan",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return atan(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

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

void init_trigonometry(py::module &m) {
  bind_sin<Variable>(m);
  bind_asin<Variable>(m);
  bind_cos<Variable>(m);
  bind_acos<Variable>(m);
  bind_tan<Variable>(m);
  bind_atan<Variable>(m);
  bind_atan2<Variable>(m);
}
