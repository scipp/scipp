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
  m.def("concatenate", py::overload_cast<ConstView, ConstView, const Dim>(&concatenate),
        py::arg("x"), py::arg("y"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Concatenate input variables, or all the variables in a supplied dataset, along the given dimension.

        Concatenation can happen in two ways:
        - Along an existing dimension, yielding a new dimension extent given by the sum of the input's extents.
        - Along a new dimension that is not contained in either of the inputs, yielding an output with one extra dimensions.

        In the case of a dataset or data array, data, coords, and masks are concatenated.
        Coords, and masks for any but the given dimension are required to match and are copied to the output without changes.
        In the case of a dataset, the output contains only items that are present in both inputs.

        :param x: First Variable, DataArray, or Dataset.
        :param y: Second Variable, DataArray, or Dataset.
        :param dim: Dimension over which to concatenate.
        :raises: If the dtype or unit does not match, or if the dimensions and shapes are incompatible.
        :return: New variable, data array, or dataset containing the concatenated data.
        :rtype: Variable, DataArray, or Dataset)");
}

template <class T> void bind_abs(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  using View = const typename T::view_type &;
  m.def("abs", [](ConstView self) { return abs(self); },
        py::arg("x"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Element-wise absolute value.

        :raises: If the dtype has no absolute value, e.g., if it is a string
        :seealso: :py:class:`scipp.norm` for vector-like dtype
        :return: Copy of the input with values replaced by the absolute values
        :rtype: Variable, DataArray, or Dataset)");
  m.def("abs", [](ConstView self, View out) {
          return abs(self, out);
        },
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
        R"(
        In-place element-wise absolute value.

        :param x: Input Variable, DataArray, or Dataset.
        :param out: Output buffer to which the absolute values will be written.
        :raises: If the dtype has no absolute value, e.g., if it is a string.
        :seealso: :py:class:`scipp.norm` for vector-like dtype.
        :return: Input with values replaced by the absolute values.
        :rtype: VariableView, DataArrayView, or DatasetView)");
}


void init_operations(py::module &m) {
  bind_flatten<Variable>(m);
  bind_flatten<DataArray>(m);
  bind_flatten<Dataset>(m);

  bind_concatenate<Variable>(m);
  bind_concatenate<DataArray>(m);
  bind_concatenate<Dataset>(m);

  bind_abs<Variable>(m);
}
