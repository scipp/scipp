// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "detail.h"
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <typename T> void bind_abs(py::module &m) {
  auto doc = Docstring()
        .description("Element-wise absolute value.")
        .raises("If the dtype has no absolute value, e.g., if it is a string.")
        .seealso(":py:class:`scipp.norm` for vector-like dtype.")
        .returns("The absolute values of the input.")
        .rtype<T>()
        .param("x", "Input variable.");
  m.def(
      "abs", [](CstViewRef<T> x) { return abs(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
  m.def(
      "abs",
      [](CstViewRef<T> x, ViewRef<T> out) {
        return abs(x, out);
      },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<View<T>>().param("out", "Output buffer.").c_str());
}

template <typename T> void bind_sqrt(py::module &m) {
  auto doc = Docstring()
        .description("Element-wise square-root.")
        .raises("If the dtype has no square-root, e.g., if it is a string.")
        .returns("The square-root values of the input.")
        .rtype<T>()
        .param("x", "Input variable.");
  m.def(
      "sqrt", [](CstViewRef<T> x) { return sqrt(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
  m.def(
      "sqrt",
      [](CstViewRef<T> x, ViewRef<T> out) {
        return sqrt(x, out);
      },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<View<T>>().param("out", "Output buffer.").c_str());
}


void init_unary(py::module &m) {
  bind_abs<Variable>(m);
  bind_sqrt<Variable>(m);
}
