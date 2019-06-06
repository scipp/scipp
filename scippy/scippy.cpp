// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <pybind11/pybind11.h>

namespace py = pybind11;

void init_dimensions(py::module &);
void init_units_neutron(py::module &);
void init_variable(py::module &);

PYBIND11_MODULE(scippy, m) {
  init_dimensions(m);
  init_units_neutron(m);
  init_variable(m);
}
