// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/choose.h"
#include "scipp/dataset/dataset.h"

#include "docstring.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

void init_choose(py::module &m) {
  m.def("choose", choose, py::arg("key"), py::arg("choices"), py::arg("dim"),
        py::call_guard<py::gil_scoped_release>(),
        Docstring()
            .description("Choose slices of choices base on coord values.")
            .returns("DataArray with data based on chosed slices.")
            .rtype<DataArray>()
            .param("key", "Values to choose.", "Variable")
            .param("choices", "Data array to choose slices from.", "DataArray")
            .param("dim", "Dimension and coord to use for slicing and choices.",
                   "Dim")
            .c_str());
}
