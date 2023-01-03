// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/element_array_variable.tcc"
#include "scipp/variable/string.h"

#include "py_object.h"
#include "pybind11.h"

namespace scipp::variable {

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(PyObject, scipp::python::PyObject)

} // namespace scipp::variable

namespace scipp::python {
REGISTER_FORMATTER(PyObject, PyObject)
} // namespace scipp::python
