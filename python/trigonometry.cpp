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

#define BIND_TRIG_FUNCTION(op)                                                 \
  template <class T> void bind_##op(py::module &m) {                           \
    auto doc = Docstring()                                                     \
                   .description("Element-wise " #op ".")                       \
                   .raises("If the unit is not a plane-angle unit, or if the " \
                           "trigonometric "                                    \
                           "function cannot be computed on the dtype, e.g., "  \
                           "if it is an integer.")                             \
                   .returns("The " #op " of the input values.")                \
                   .rtype<T>()                                                 \
                   .template param<T>("x", "Input values.");           \
    m.def(#op, [](CstViewRef<T> self) { return op(self); }, py::arg("x"),      \
          py::call_guard<py::gil_scoped_release>(), doc.c_str());              \
    m.def(                                                                     \
        #op, [](CstViewRef<T> self, ViewRef<T> out) { return op(self, out); }, \
        py::arg("x"), py::arg("out"),                                          \
        py::call_guard<py::gil_scoped_release>(),                              \
        doc.template rtype<View<T>>().template param<T>("out", "Output buffer").c_str());  \
    doc = doc.description("Element-wise a" #op ".")                            \
              .returns("The a" #op " of the input values.")             \
              .raises("If the unit is not dimensionless.");                    \
    m.def("a" #op, [](CstViewRef<T> self) { return a##op(self); },             \
          py::arg("x"), py::call_guard<py::gil_scoped_release>(),              \
          doc.c_str());                                                        \
    m.def(                                                                     \
        "a" #op,                                                               \
        [](CstViewRef<T> self, ViewRef<T> out) { return a##op(self, out); },   \
        py::arg("x"), py::arg("out"),                                          \
        py::call_guard<py::gil_scoped_release>(),                              \
        doc.template rtype<View<T>>().template param<T>("out", "Output buffer").c_str());  \
  }

BIND_TRIG_FUNCTION(sin)
BIND_TRIG_FUNCTION(cos)
BIND_TRIG_FUNCTION(tan)

template <class T> void bind_atan2(py::module &m) {
  auto doc =
      Docstring()
          .description("Element-wise atan2.")
          .raises("If the unit is not a plane-angle unit, or if the atan2 "
                  "function cannot be computed on the dtype, e.g., if it is an "
                  "integer.")
          .returns("The atan2 values of the input.")
          .rtype<T>()
          .template param<T>("y", "Variable containing the y values.")
          .template param<T>("x", "Variable containing the x values.");
  m.def("atan2", [](CstViewRef<T> y, CstViewRef<T> x) { return atan2(y, x); },
        py::arg("y"), py::arg("x"), py::call_guard<py::gil_scoped_release>(),
        doc.c_str());
  m.def("atan2",
        [](CstViewRef<T> y, CstViewRef<T> x, View<T> out) {
          return atan2(y, x, out);
        },
        py::arg("y"), py::arg("x"), py::arg("out"),
        py::call_guard<py::gil_scoped_release>(),
        doc.template rtype<View<T>>().template param<T>("out", "Output buffer").c_str());
}

void init_trigonometry(py::module &m) {
  bind_sin<Variable>(m);
  bind_cos<Variable>(m);
  bind_tan<Variable>(m);
  bind_atan2<Variable>(m);
}
