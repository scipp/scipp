// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/comparison.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> Docstring docstring_comparison(const std::string op) {
  return Docstring()
      .description(
          "Comparison returning the truth value of " + op +
          " element-wise.\n\nNote and warning: If one or both of the operators "
          "have variances (uncertainties) there are ignored silently, i.e., "
          "comparison is based exclusively on the values.")
      .raises("If the units of inputs are not the same, or if the dtypes of "
              "inputs are not double precision floats.")
      .returns("Booleans that are true if " + op + ".")
      .rtype<T>()
      .template param<T>("x", "Input left operand.")
      .template param<T>("y", "Input right operand.");
}

template <typename T> void bind_less(py::module &m) {
  m.def(
      "less",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return less(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x < y)").c_str());
}

template <typename T> void bind_greater(py::module &m) {
  m.def(
      "greater",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return greater(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x > y)").c_str());
}

template <typename T> void bind_less_equal(py::module &m) {
  m.def(
      "less_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return less_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x <= y)").c_str());
}

template <typename T> void bind_greater_equal(py::module &m) {
  m.def(
      "greater_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return greater_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x >= y)").c_str());
}

template <typename T> void bind_equal(py::module &m) {
  m.def(
      "equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x == y)").c_str());
}

template <typename T> void bind_not_equal(py::module &m) {
  m.def(
      "not_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return not_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x != y)").c_str());
}

void init_comparison(py::module &m) {
  bind_less<Variable>(m);
  bind_greater<Variable>(m);
  bind_less_equal<Variable>(m);
  bind_greater_equal<Variable>(m);
  bind_equal<Variable>(m);
  bind_not_equal<Variable>(m);
}
