// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "py_object.h"

namespace nb = nanobind;

namespace scipp::python {

PyObject::~PyObject() {
  nb::gil_scoped_acquire acquire;
  m_object = nb::object();
}

PyObject::PyObject(const nb::object &object) {
  nb::gil_scoped_acquire acquire;
  m_object = object; // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

bool PyObject::operator==(const PyObject &other) const {
  // Similar to above, re-acquiring GIL here due to segfault in Python C API
  // (PyObject_RichCompare).
  nb::gil_scoped_acquire acquire;
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
    nb::gil_scoped_acquire acquire;
    nb::module_ copy = nb::module_::import_("copy");
    nb::object deepcopy = copy.attr("deepcopy");
    return {deepcopy(object)};
  } else {
    return {object};
  }
}

std::ostream &operator<<(std::ostream &os, const PyObject &obj) {
  return os << to_string(obj);
}

std::string to_string(const PyObject &obj) {
  nb::gil_scoped_acquire gil_{};
  return nb::cast<std::string>(nb::str(obj.to_pybind()));
}

} // namespace scipp::python
