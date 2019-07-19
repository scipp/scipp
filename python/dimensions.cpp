// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dimensions.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

void init_dimensions(py::module &m) {
  py::class_<Dimensions>(m, "Dimensions")
      .def_property_readonly_static(
          "Sparse", [](py::object /* self */) { return Dimensions::Sparse; },
          "Dummy label to use when specifying the shape for sparse data.");
}
