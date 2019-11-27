// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/core/dataset.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

template <class T> void bind_flatten(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  m.def("flatten", py::overload_cast<ConstView, const Dim>(&flatten),
        py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Flatten the specified dimension into to sparse dimension, equivalent to summing dense data.

        :param x: Variable, DataArray, or Dataset to flatten.
        :param dim: Dimension over which to flatten.
        :raises: If the dimension does not exist, or if x has no sparse dimension
        :seealso: :py:class:`scipp.sum`
        :return: New variable, data array, or dataset containing the flattened data.
        :rtype: Variable, DataArray, or Dataset)");
}

void init_operations(py::module &m) {
  bind_flatten<Variable>(m);
  bind_flatten<DataArray>(m);
  bind_flatten<Dataset>(m);
}
