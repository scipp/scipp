// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

#define BIND_TRIGONOMETRIC_FUNCTION(op, module, T)                             \
  head = std::string("Element-wise ") + #op + ".\n" +                          \
         ":param x: Input Variable, DataArray, or Dataset.\n";                 \
  tail = std::string(":raises: If the unit is not a plane-angle unit, or if "  \
                     "the dtype has no ") +                                    \
         #op +                                                                 \
         ", e.g., if it is an integer.\n"                                      \
         ":return: Variable containing the " +                                 \
         #op + ".\n";                                                          \
  rtype = ":rtype: Variable, DataArray, or Dataset.";                          \
  rtype_out = ":rtype: VariableView, DataArrayView, or DatasetView.";          \
  param_out = std::string(":param out: Output buffer to which the ") + #op +   \
              " values will be written.\n";                                    \
  aop = std::string("a") + #op;                                                \
  m.def(#op, [](T::const_view_type self) { return op(self); }, py::arg("x"),   \
        py::call_guard<py::gil_scoped_release>(),                              \
        &((head + tail + rtype)[0]));                                          \
  m.def(                                                                       \
      #op,                                                                     \
      [](T::const_view_type self, T::view_type out) { return op(self, out); }, \
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),  \
      &((head + param_out + tail + rtype_out)[0]));                            \
  m.def(&(aop[0]), [](T::const_view_type self) { return a##op(self); },        \
        py::arg("x"), py::call_guard<py::gil_scoped_release>(),                \
        &((head + tail + rtype)[0]));                                          \
  m.def(&(aop[0]),                                                             \
        [](T::const_view_type self, T::view_type out) {                        \
          return a##op(self, out);                                             \
        },                                                                     \
        py::arg("x"), py::arg("out"),                                          \
        py::call_guard<py::gil_scoped_release>(),                              \
        &((head + param_out + tail + rtype_out)[0]));

void init_trigonometry(py::module &m) {

  std::string head, tail, rtype, rtype_out, param_out, aop;

  BIND_TRIGONOMETRIC_FUNCTION(sin, m, Variable)
  BIND_TRIGONOMETRIC_FUNCTION(cos, m, Variable)
  BIND_TRIGONOMETRIC_FUNCTION(tan, m, Variable)
}
