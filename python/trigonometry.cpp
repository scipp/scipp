// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "docstring.h"
#include "pybind11.h"

#include "scipp/variable/trigonometry.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> Docstring docstring_trig(const std::string &op) {
  return Docstring()
      .description("Element-wise " + op + ".")
      .raises("If the unit is not a plane-angle unit, or if the " + op +
              " function cannot be computed on the dtype, e.g., if it is an "
              "integer.")
      .returns("The " + op + " values of the input.")
      .rtype<T>()
      .template param<T>("x", "Input data.");
}

template <class T> void bind_sin(py::module &m) {
  auto doc = docstring_trig<T>("sin");
  m.def(
      "sin", [](const typename T::const_view_type &x) { return sin(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "sin",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return sin(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer")
          .c_str());
}

template <class T> void bind_asin(py::module &m) {
  auto doc =
      docstring_trig<T>("asin").raises("If the unit is not dimensionless.");
  m.def(
      "asin", [](const typename T::const_view_type &x) { return asin(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "asin",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return asin(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer")
          .c_str());
}

template <class T> void bind_cos(py::module &m) {
  auto doc = docstring_trig<T>("cos");
  m.def(
      "cos", [](const typename T::const_view_type &x) { return cos(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "cos",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return cos(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer")
          .c_str());
}

template <class T> void bind_acos(py::module &m) {
  auto doc =
      docstring_trig<T>("acos").raises("If the unit is not dimensionless.");
  m.def(
      "acos", [](const typename T::const_view_type &x) { return acos(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "acos",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return acos(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer")
          .c_str());
}

template <class T> void bind_tan(py::module &m) {
  auto doc = docstring_trig<T>("tan");
  m.def(
      "tan", [](const typename T::const_view_type &x) { return tan(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "tan",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return tan(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer")
          .c_str());
}

template <class T> void bind_atan(py::module &m) {
  auto doc =
      docstring_trig<T>("atan").raises("If the unit is not dimensionless.");
  m.def(
      "atan", [](const typename T::const_view_type &x) { return atan(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "atan",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return atan(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer")
          .c_str());
}

template <class T> void bind_atan2(py::module &m) {
  auto doc =
      Docstring()
          .description("Element-wise atan2.")
          .raises("If the unit is not a plane-angle unit, or if the atan2 "
                  "function cannot be computed on the dtype, e.g., if it is an "
                  "integer.")
          .returns("The atan2 values of the input.")
          .rtype<T>()
          .template param<T>("y", "Input y values.")
          .template param<T>("x", "Input x values.");
  m.def(
      "atan2",
      [](const typename T::const_view_type &y,
         const typename T::const_view_type &x) { return atan2(y, x); },
      py::arg("y"), py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
  m.def(
      "atan2",
      [](const typename T::const_view_type &y,
         const typename T::const_view_type &x,
         typename T::view_type out) { return atan2(y, x, out); },
      py::arg("y"), py::arg("x"), py::arg("out"),
      py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer")
          .c_str());
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
