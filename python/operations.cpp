// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T>
using ConstView = const typename T::const_view_type &;

template <class T>
using View = const typename T::view_type &;


template <class T> void bind_flatten(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  m.def("flatten", py::overload_cast<ConstView, const Dim>(&flatten),
        py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Flatten the specified dimension into event lists, equivalent to summing dense data.

        :param x: Variable, DataArray, or Dataset to flatten.
        :param dim: Dimension over which to flatten.
        :raises: If the dimension does not exist, or if x does not contain event lists
        :seealso: :py:class:`scipp.sum`
        :return: New variable, data array, or dataset containing the flattened data.
        :rtype: Variable, DataArray, or Dataset)");
}

template <class T> void bind_concatenate(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  auto doc = Docstring()
        .description(R"(
        Concatenate input data array along the given dimension.

        Concatenation can happen in two ways:
         - Along an existing dimension, yielding a new dimension extent given by the sum of the input's extents.
         - Along a new dimension that is not contained in either of the inputs, yielding an output with one extra dimensions.

        In the case of a data array or dataset, the coords, and masks are also concatenated.
        Coords, and masks for any but the given dimension are required to match and are copied to the output without changes.)")
        .raises("If the dtype or unit does not match, or if the dimensions and shapes are incompatible.")
        .returns("The absolute values of the input.")
        .rtype<T>()
        .param("x", "Left hand side input.")
        .param("y", "Right hand side input.")
        .param("dim", "Dimension along which to concatenate.");
  m.def("concatenate",
    [](ConstView x, ConstView y, const Dim dim) { return concatenate(x, y, dim); },
        // py::overload_cast<const DataArrayConstView &,
        //                   const DataArrayConstView &, const Dim>(&concatenate),
        py::arg("x"), py::arg("y"), py::arg("dim"),
        py::call_guard<py::gil_scoped_release>(),
        doc.c_str());
}

template <class T> Docstring docstring_abs() {
  return Docstring()
        .description("Element-wise absolute value.")
        .raises("If the dtype has no absolute value, e.g., if it is a string.")
        .seealso(":py:class:`scipp.norm` for vector-like dtype.")
        .returns("The absolute values of the input.")
        // .rtype(T())
        .rtype<T>()
        .param("x", "Input variable.");
}

template <typename T> void bind_abs(py::module &m) {
  // using ConstView = const typename T::const_view_type &;
  // using V = typename T::view_type;
  m.def(
      "abs", [](const typename T::const_view_type & self) { return abs(self); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      docstring_abs<T>().c_str());
 }

template <typename T> void bind_abs_out(py::module &m) {
  m.def(
      "abs",
      [](ConstView self, View out) {
        return abs(self, out);
      },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<T>().param("out", "Output buffer.").c_str());
}

template <typename T> void bind_dot(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  using View = const typename T::view_type &;
  auto doc = Docstring()
        .description("Element-wise dot product.")
        .raises("If the dtype of the input is not vector_3_float64.")
        .returns("The dot product of the input vectors.")
        .rtype<T>()
        .param("x", "Input left hand side operand.")
        .param("y", "Input right hand side operand.");
  m.def(
      "dot", [](ConstView x, ConstView y) { return dot(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
  // m.def(
  //     "dot",
  //     [](ConstView x, ConstView y, View out) {
  //       return dot(x, y, out);
  //     },
  //     py::arg("x"), py::arg("y"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
  //     doc.param("out", "Output buffer.").c_str());
}

void init_operations(py::module &m) {
  bind_flatten<Variable>(m);
  bind_flatten<DataArray>(m);
  bind_flatten<Dataset>(m);

  bind_concatenate<Variable>(m);
  bind_concatenate<DataArray>(m);
  bind_concatenate<Dataset>(m);

  bind_abs<Variable>(m);

  bind_dot<Variable>(m);

}
