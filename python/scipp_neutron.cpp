// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

namespace py = pybind11;

void init_neutron(py::module &);

PYBIND11_MODULE(_scipp_neutron, m) { init_neutron(m); }
