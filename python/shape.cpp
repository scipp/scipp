// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#include "scipp/variable/shape.h"
#include "docstring.h"
#include "pybind11.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

namespace {

template <class T> void bind_concatenate(py::module &m) {
  auto doc = Docstring()
                 .description(R"(
Concatenate input data array along the given dimension.

Concatenation can happen in two ways:
 - Along an existing dimension, yielding a new dimension extent given by the sum
   of the input's extents.
 - Along a new dimension that is not contained in either of the inputs, yielding
   an output with one extra dimensions.

In the case of a data array or dataset, the coords, and masks are also
concatenated.
Coords, and masks for any but the given dimension are required to match and are
copied to the output without changes.)")
                 .raises("If the dtype or unit does not match, or if the "
                         "dimensions and shapes are incompatible.")
                 .returns("The absolute values of the input.")
                 .rtype<T>()
                 .template param<T>("x", "Left hand side input.")
                 .template param<T>("y", "Right hand side input.")
                 .param("dim", "Dimension along which to concatenate.", "Dim");
  m.def(
      "concatenate",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y,
         const Dim dim) { return concatenate(x, y, dim); },
      py::arg("x"), py::arg("y"), py::arg("dim"),
      py::call_guard<py::gil_scoped_release>(), doc.c_str());
}
template <class T> void bind_reshape(pybind11::module &mod) {
  mod.def(
      "reshape",
      [](const T &self, const std::vector<Dim> &labels,
         const py::tuple &shape) {
        Dimensions dims(labels, shape.cast<std::vector<scipp::index>>());
        return reshape(self, dims);
      },
      py::arg("x"), py::arg("dims"), py::arg("shape"),
      Docstring()
          .description("Reshape a variable.")
          .raises("If the volume of the old shape is not equal to the volume "
                  "of the new shape.")
          .returns("New variable with requested dimension labels and shape.")
          .rtype("Variable")
          .param("x", "Variable to reshape.", "Variable.")
          .param("dims", "List of new dimensions.", "list")
          .param("shape", "New extents in each dimension.", "list")
          .c_str());
}

template <class T> void bind_transpose(pybind11::module &mod) {
  mod.def(
      "transpose",
      [](const T &self, const std::vector<Dim> &dims) {
        return transpose(self, dims);
      },
      py::arg("x"), py::arg("dims") = std::vector<Dim>{},
      Docstring()
          .description("Transpose dimensions of a variable.")
          .raises(
              "If dims set does not match existing dimensions or is not empty")
          .returns("New variable with requested dimension order.")
          .rtype("Variable")
          .param("x", "Variable to transpose.", "Variable.")
          .param("dims",
                 "List of dimensions in desired order. If default, reverses "
                 "existing order.",
                 "list")
          .c_str());
}
} // namespace

void init_shape(py::module &m) {
  bind_concatenate<Variable>(m);
  bind_concatenate<DataArray>(m);
  bind_concatenate<Dataset>(m);
  bind_reshape<Variable>(m);
  bind_reshape<VariableView>(m);
  bind_transpose<Variable>(m);
  bind_transpose<VariableView>(m);
}
