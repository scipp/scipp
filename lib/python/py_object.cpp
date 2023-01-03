// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
  py::gil_scoped_acquire acquire;
  m_object = object; // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

bool PyObject::operator==(const PyObject &other) const {
  // Similar to above, re-acquiring GIL here due to segfault in Python C API
  // (PyObject_RichCompare).
  py::gil_scoped_acquire acquire;
  return to_pybind().equal(other.to_pybind());
}

PyObject copy(const PyObject &obj) {
  const auto &object = obj.to_pybind();
  if (object) {
    // It is essential to acquire the GIL here. Calling Python code otherwise
    // causes a segfault if the GIL has been released previously. Since this
    // copy operation is called by anything that copies variables, this includes
    // almost every C++ function with Python bindings because we typically do
    // release the GIL everywhere.
    py::gil_scoped_acquire acquire;
    py::module copy = py::module::import("copy");
    py::object deepcopy = copy.attr("deepcopy");
    return {deepcopy(object)};
  } else {
    return {object};
  }
}

std::ostream &operator<<(std::ostream &os, const PyObject &obj) {
  return os << to_string(obj);
}

std::string to_string(const PyObject &obj) {
  py::gil_scoped_acquire gil_{};
  return py::str(obj.to_pybind());
}

} // namespace scipp::python
