// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> void bind_min(py::module &m) {
  m.def(
      "min", [](const T &x) { return min(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "min",
      [](const T &x, const std::string &dim) { return min(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_max(py::module &m) {
  m.def(
      "max", [](const T &x) { return max(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "max",
      [](const T &x, const std::string &dim) { return max(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nanmin(py::module &m) {
  m.def(
      "nanmin", [](const T &x) { return nanmin(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "nanmin",
      [](const T &x, const std::string &dim) { return nanmin(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nanmax(py::module &m) {
  m.def(
      "nanmax", [](const T &x) { return nanmax(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "nanmax",
      [](const T &x, const std::string &dim) { return nanmax(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

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
  bind_min<Variable>(m);
  bind_max<Variable>(m);
  bind_nanmin<Variable>(m);
  bind_nanmax<Variable>(m);
  bind_all<Variable>(m);
  bind_any<Variable>(m);
}
