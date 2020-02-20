// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "py_object.h"

namespace py = pybind11;

namespace scipp::python {

PyObject::~PyObject() {
  py::gil_scoped_acquire acquire;
  m_object = py::object();
}

PyObject::PyObject(const py::object &object) {
  if (object) {
    // It is essential to acquire the GIL here. Calling Python code otherwise
    // causes a segfault if the GIL has been released previously. Since this
    // copy operation is called by anything that copies variables, this includes
    // almost every C++ function with Python bindings because we typically do
    // release the GIL everywhere.
    py::gil_scoped_acquire acquire;
    py::module copy = py::module::import("copy");
    py::object deepcopy = copy.attr("deepcopy");
    m_object = deepcopy(object);
  } else {
    m_object = object;
  }
}

bool PyObject::operator==(const PyObject &other) const {
  // Similar to above, re-acquiring GIL here due to segfault in Python C API
  // (PyObject_RichCompare).
  py::gil_scoped_acquire acquire;
  return to_pybind().equal(other.to_pybind());
}

} // namespace scipp::python
