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

template <class T> void bind_abs(py::module &m) {
  auto doc = Docstring()
        .description("Element-wise absolute value.")
        .raises("If the dtype has no absolute value, e.g., if it is a string.")
        .seealso(":py:class:`scipp.norm` for vector-like dtype.")
        .returns("Variable with the absolute values of the input.")
        .rtype("Variable")
        .param("x", "Input variable.");
  m.def(
      "abs", [](const VariableConstView &self) { return abs(self); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
  m.def(
      "abs",
      [](const VariableConstView &self, const VariableView &out) {
        return abs(self, out);
      },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.param("out", "Output buffer.").c_str());
}

void init_operations(py::module &m) {
  bind_flatten<Variable>(m);
  bind_flatten<DataArray>(m);
  bind_flatten<Dataset>(m);

  bind_abs<Variable>(m);

}
