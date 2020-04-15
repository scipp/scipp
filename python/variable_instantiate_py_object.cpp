// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/variable.tcc"

#include "py_object.h"
#include "pybind11.h"

namespace scipp::variable {

INSTANTIATE_VARIABLE(PyObject, scipp::python::PyObject)

} // namespace scipp::variable
