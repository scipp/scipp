// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/variable.tcc"

#include "pybind11.h"

namespace py = pybind11;

namespace scipp::core {

INSTANTIATE_VARIABLE(py::object)

} // namespace scipp::core
