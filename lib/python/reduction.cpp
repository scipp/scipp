// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> void bind_all(py::module &m) {
  m.def(
      "all", [](const T &x) { return all(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "all",
      [](const T &x, const std::string &dim) { return all(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_any(py::module &m) {
  m.def(
      "any", [](const T &x) { return any(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "any",
      [](const T &x, const std::string &dim) { return any(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

void init_reduction(py::module &m) {
  bind_all<Variable>(m);
  bind_any<Variable>(m);
}
