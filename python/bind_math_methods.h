// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPPY_BIND_MATH_METHODS_H
#define SCIPPY_BIND_MATH_METHODS_H

#include "pybind11.h"

namespace py = pybind11;

template <class T, class... Ignored>
void bind_math_methods(pybind11::class_<T, Ignored...> &c) {
  c.def(py::self == py::self, py::call_guard<py::gil_scoped_release>());
  c.def(py::self != py::self, py::call_guard<py::gil_scoped_release>());
  c.def(py::self += py::self, py::call_guard<py::gil_scoped_release>());
  c.def(py::self -= py::self, py::call_guard<py::gil_scoped_release>());
  c.def(py::self *= py::self, py::call_guard<py::gil_scoped_release>());
  c.def(py::self /= py::self, py::call_guard<py::gil_scoped_release>());
}

#endif // SCIPPY_BIND_MATH_METHODS_H
