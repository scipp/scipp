// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "detail.h"
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
              "function cannot be computed on the dtype, e.g., if it is an "
              "integer.")
      .returns("The " + op + " values of the input.")
      .rtype<T>()
      .template param<T>("x", "Input data.");
}

template <class T> void bind_sin(py::module &m) {
  auto doc = docstring_trig<T>("sin");
  m.def("sin", [](CstViewRef<T> x) { return sin(x); }, py::arg("x"),
        py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def("sin", [](CstViewRef<T> x, ViewRef<T> out) { return sin(x, out); },
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
        doc.template rtype<View<T>>()
            .template param<T>("out", "Output buffer")
            .c_str());
}

template <class T> void bind_asin(py::module &m) {
  auto doc =
      docstring_trig<T>("asin").raises("If the unit is not dimensionless.");
  m.def("asin", [](CstViewRef<T> x) { return asin(x); }, py::arg("x"),
        py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def("asin", [](CstViewRef<T> x, ViewRef<T> out) { return asin(x, out); },
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
        doc.template rtype<View<T>>()
            .template param<T>("out", "Output buffer")
            .c_str());
}

template <class T> void bind_cos(py::module &m) {
  auto doc = docstring_trig<T>("cos");
  m.def("cos", [](CstViewRef<T> x) { return cos(x); }, py::arg("x"),
        py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def("cos", [](CstViewRef<T> x, ViewRef<T> out) { return cos(x, out); },
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
        doc.template rtype<View<T>>()
            .template param<T>("out", "Output buffer")
            .c_str());
}

template <class T> void bind_acos(py::module &m) {
  auto doc =
      docstring_trig<T>("acos").raises("If the unit is not dimensionless.");
  m.def("acos", [](CstViewRef<T> x) { return acos(x); }, py::arg("x"),
        py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def("acos", [](CstViewRef<T> x, ViewRef<T> out) { return acos(x, out); },
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
        doc.template rtype<View<T>>()
            .template param<T>("out", "Output buffer")
            .c_str());
}

template <class T> void bind_tan(py::module &m) {
  auto doc = docstring_trig<T>("tan");
  m.def("tan", [](CstViewRef<T> x) { return tan(x); }, py::arg("x"),
        py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def("tan", [](CstViewRef<T> x, ViewRef<T> out) { return tan(x, out); },
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
        doc.template rtype<View<T>>()
            .template param<T>("out", "Output buffer")
            .c_str());
}

template <class T> void bind_atan(py::module &m) {
  auto doc =
      docstring_trig<T>("atan").raises("If the unit is not dimensionless.");
  m.def("atan", [](CstViewRef<T> x) { return atan(x); }, py::arg("x"),
        py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def("atan", [](CstViewRef<T> x, ViewRef<T> out) { return atan(x, out); },
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
        doc.template rtype<View<T>>()
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
  m.def("atan2", [](CstViewRef<T> y, CstViewRef<T> x) { return atan2(y, x); },
        py::arg("y"), py::arg("x"), py::call_guard<py::gil_scoped_release>(),
        doc.c_str());
  m.def("atan2",
        [](CstViewRef<T> y, CstViewRef<T> x, View<T> out) {
          return atan2(y, x, out);
        },
        py::arg("y"), py::arg("x"), py::arg("out"),
        py::call_guard<py::gil_scoped_release>(),
        doc.template rtype<View<T>>()
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
