// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/shape.h"
#include "scipp/dataset/sort.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/util.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

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

template <typename T> void bind_dot(py::module &m) {
  auto doc = Docstring()
                 .description("Element-wise dot product.")
                 .raises("If the dtype of the input is not vector_3_float64.")
                 .returns("The dot product of the input vectors.")
                 .rtype<T>()
                 .template param<T>("x", "Input left hand side operand.")
                 .template param<T>("y", "Input right hand side operand.");
  m.def(
      "dot",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return dot(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
}

template <typename T> void bind_sort(py::module &m) {
  auto doc =
      Docstring()
          .description("Sort variable along a dimension by a sort key.")
          .raises("If the key is invalid, e.g., if it has not exactly one "
                  "dimension, or if its dtype is not sortable.")
          .returns("The sorted equivalent of the input.")
          .rtype<T>()
          .template param<T>("x", "Data to be sorted")
          .template param<T>("key", "Sort key.");
  m.def(
      "sort",
      [](const typename T::const_view_type &x,
         const typename Variable::const_view_type &key) {
        return sort(x, key);
      },
      py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
}

template <typename T> void bind_sort_dim(py::module &m) {
  auto doc =
      Docstring()
          .description("Sort data array along a dimension by the coordinate "
                       "values for that dimension.")
          .raises(
              "If the key is invalid, e.g., if the dimension does not exist.")
          .returns("The sorted equivalent of the input.")
          .rtype<T>()
          .template param<T>("x", "Data to be sorted")
          .param("dim", "Dimension to sort along.", "Dim");
  m.def(
      "sort",
      [](const typename T::const_view_type &x, const Dim &dim) {
        return sort(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
}

template <typename T> void bind_contains_events(py::module &m) {
  m.def(
      "contains_events",
      [](const typename T::const_view_type &x) { return contains_events(x); },
      Docstring()
          .description("Return `True` if the input contains event data. "
                       "Note that data may be stored as a scalar.\n"
                       "In the case of a DataArray, this "
                       "returns `True` if any coord contains events.")
          .returns("`True` or `False`.")
          .rtype("bool")
          .param<T>("x", "Input data.")
          .c_str());
}

void init_operations(py::module &m) {
  bind_concatenate<Variable>(m);
  bind_concatenate<DataArray>(m);
  bind_concatenate<Dataset>(m);

  bind_dot<Variable>(m);

  bind_sort<Variable>(m);
  bind_sort<DataArray>(m);
  bind_sort<Dataset>(m);
  bind_sort_dim<DataArray>(m);
  bind_sort_dim<Dataset>(m);

  bind_contains_events<Variable>(m);
  bind_contains_events<DataArray>(m);

  m.def("values", variable::values,
        Docstring()
            .description("Return the variable without variances.")
            .seealso(":py:func:`scipp.variances`")
            .c_str(),
        py::call_guard<py::gil_scoped_release>());
  m.def("variances", variable::variances,
        Docstring()
            .description("Return variable containing the variances of the "
                         "input as values.")
            .seealso(":py:func:`scipp.values`")
            .c_str(),
        py::call_guard<py::gil_scoped_release>());
}
