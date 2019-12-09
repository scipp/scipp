// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/variable.tcc"

#include "py_object.h"
#include "pybind11.h"

namespace scipp::core {

INSTANTIATE_VARIABLE(scipp::python::PyObject)

} // namespace scipp::core
