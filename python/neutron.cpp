// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/neutron/convert.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::neutron;

namespace py = pybind11;

void init_neutron(py::module &m) {
  auto neutron = m.def_submodule("neutron");

  neutron.def("convert", convert, py::call_guard<py::gil_scoped_release>(),
              "Convert coordinates from one dimension (unit) to another.");
}
