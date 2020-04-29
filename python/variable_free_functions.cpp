// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

// #include "scipp/units/unit.h"

// #include "scipp/core/dtype.h"
// #include "scipp/core/except.h"
// #include "scipp/core/tag_util.h"

// #include "scipp/variable/comparison.h"
#include "scipp/variable/operations.h"
// #include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

// // #include "scipp/dataset/dataset.h"
// #include "scipp/dataset/sort.h"

// #include "bind_data_access.h"
// #include "bind_operators.h"
// #include "bind_slice_methods.h"
// #include "dtype.h"
// #include "make_variable.h"
// #include "numpy.h"
// #include "py_object.h"
#include "pybind11.h"
// #include "rename.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

void init_variable_free_functions(py::module &m) {

  m.def("reshape",
        [](const VariableView &self, const std::vector<Dim> &labels,
           const py::tuple &shape) {
          Dimensions dims(labels, shape.cast<std::vector<scipp::index>>());
          return self.reshape(dims);
        },
        py::arg("x"), py::arg("dims"), py::arg("shape"), R"(
        Reshape a variable.

        :param x: Data to reshape.
        :param dims: List of new dimensions.
        :param shape: New extents in each dimension.
        :raises: If the volume of the old shape is not equal to the volume of the new shape.
        :return: New variable with requested dimension labels and shape.
        :rtype: Variable)");

  m.def("split",
        py::overload_cast<const Variable &, const Dim,
                          const std::vector<scipp::index> &>(&split),
        py::call_guard<py::gil_scoped_release>(),
        "Split a Variable along a given Dimension.");


}
