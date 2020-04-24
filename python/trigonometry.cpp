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

#define BIND_TRIGONOMETRIC_FUNCTION(op, module, T)        \
  const std::string head = std::string("Element-wise ") + #op +".\n" + \
    ":param x: Input Variable, DataArray, or Dataset.\n"; \
  const std::string tail = std::string(":raises: If the unit is not a plane-angle unit, or if the dtype has no ") + \
    #op + ", e.g., if it is an integer.\n" \
    ":return: Variable containing the " + #op +".\n"; \
  m.def(#op, [](T::const_view_type self) { return op(self); },         \
        py::arg("x"), py::call_guard<py::gil_scoped_release>(),         \
        &((head + tail + ":rtype: Variable, DataArray, or Dataset.")[0]));              \
  m.def(#op, [](T::const_view_type self, T::view_type out) {           \
          return op(self, out);                                         \
        },                                                              \
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),\
        &((head + ":param out: Output buffer to which the " + #op + " values will be written.\n" + tail + ":rtype: VariableView, DataArrayView, or DatasetView.")[0]));


// template <class T> void bind_sin(py::module &m) {

//   using ConstView = const typename T::const_view_type &;
//   using View = const typename T::view_type &;
//   const std::string description = R"(
//         Element-wise sine.)";

//   m.def("sin", [](ConstView self) { return sin(self); },
//         py::arg("x"), py::call_guard<py::gil_scoped_release>(),
//         &((description + R"(
//         :param x: Input Variable, DataArray, or Dataset.
//         :raises: If the unit is not a plane-angle unit, or if the dtype has no
//         sin, e.g., if it is an integer
//         :return: Variable containing the sin.
//         :rtype: Variable, DataArray, or Dataset)")[0]));
//   m.def("sin", [](ConstView self, View out) {
//           return sin(self, out);
//         },
//         py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
//         &((description + R"( (in-place))" + R"(
//         :param x: Input Variable, DataArray, or Dataset.
//         :param out: Output buffer to which the sin values will be written.
//         :raises: If the unit is not a plane-angle unit, or if the dtype has no
//         sin, e.g., if it is an integer
//         :return: sin of input values.
//         :rtype: VariableView, DataArrayView, or DatasetView)")[0]));
// }

// template <class T> void bind_cos(py::module &m) {

//   using ConstView = const typename T::const_view_type &;
//   using View = const typename T::view_type &;
//   const std::string description = R"(
//         Element-wise cosine.)";

//   m.def("cos", [](ConstView self) { return cos(self); },
//         py::arg("x"), py::call_guard<py::gil_scoped_release>(),
//         &((description + R"(
//         :param x: Input Variable, DataArray, or Dataset.
//         :raises: If the unit is not a plane-angle unit, or if the dtype has no
//         cos, e.g., if it is an integer
//         :return: Variable containing the cos.
//         :rtype: Variable, DataArray, or Dataset)")[0]));
//   m.def("cos", [](ConstView self, View out) {
//           return cos(self, out);
//         },
//         py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
//         &((description + R"( (in-place))" + R"(
//         :param x: Input Variable, DataArray, or Dataset.
//         :param out: Output buffer to which the cos values will be written.
//         :raises: If the unit is not a plane-angle unit, or if the dtype has no
//         cos, e.g., if it is an integer
//         :return: cos of input values.
//         :rtype: VariableView, DataArrayView, or DatasetView)")[0]));
// }


void init_trigonometry(py::module &m) {

  BIND_TRIGONOMETRIC_FUNCTION(sin, m, Variable)
  BIND_TRIGONOMETRIC_FUNCTION(cos, m, Variable)

  // bind_cos<Variable>(m);

}
