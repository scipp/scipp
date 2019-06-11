// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPPY_BIND_MATH_METHODS_H
#define SCIPPY_BIND_MATH_METHODS_H

#include "dataset.h"
#include "numpy.h"
#include "pybind11.h"
#include "scipp/core/dtype.h"
#include "tag_util.h"
#include "variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::core;

template <class T, class... Ignored>
void bind_math_methods(pybind11::class_<T, Ignored...> &c) {
  c.def(py::self += py::self, py::call_guard<py::gil_scoped_release>());
  c.def(py::self *= py::self, py::call_guard<py::gil_scoped_release>());
}

#endif // SCIPPY_BIND_MATH_METHODS_H
