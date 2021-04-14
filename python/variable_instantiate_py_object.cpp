// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/string.h"
#include "scipp/variable/variable.tcc"

#include "py_object.h"
#include "pybind11.h"

namespace scipp::variable {

INSTANTIATE_VARIABLE(PyObject, scipp::python::PyObject)

} // namespace scipp::variable

namespace scipp::python {
namespace {
// Insert classes from scipp::python into formatting registry. The objects
// themselves do nothing, but the constructor call with comma operator does the
// insertion.
auto register_python_types(
    (variable::formatterRegistry().emplace(
         dtype<PyObject>, std::make_unique<variable::Formatter<PyObject>>()),
     0));
} // namespace
} // namespace scipp::python
