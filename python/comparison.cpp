// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/comparison.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <typename T> void bind_less(py::module &m) {
  m.def(
      "less",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return less(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_greater(py::module &m) {
  m.def(
      "greater",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return greater(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_less_equal(py::module &m) {
  m.def(
      "less_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return less_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_greater_equal(py::module &m) {
  m.def(
      "greater_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return greater_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_equal(py::module &m) {
  m.def(
      "equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_not_equal(py::module &m) {
  m.def(
      "not_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return not_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_is_approx(py::module &m) {
  m.def(
      "is_approx",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y,
         const typename T::const_view_type &tol) {
        return is_approx(x, y, tol);
      },
      py::arg("x"), py::arg("y"), py::arg("tol"),
      py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_is_equal(py::module &m) {
  m.def(
      "is_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return x == y; },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

void init_comparison(py::module &m) {
  bind_less<Variable>(m);
  bind_greater<Variable>(m);
  bind_less_equal<Variable>(m);
  bind_greater_equal<Variable>(m);
  bind_equal<Variable>(m);
  bind_not_equal<Variable>(m);
  bind_is_approx<Variable>(m);
  bind_is_equal<Variable>(m);
  bind_is_equal<Dataset>(m);
  bind_is_equal<DataArray>(m);
}
