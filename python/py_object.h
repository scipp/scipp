// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_PYTHON_PY_OBJECT_H
#define SCIPP_PYTHON_PY_OBJECT_H

#include "pybind11.h"

namespace py = pybind11;

namespace scipp::python {

/// Wrapper around pybind11::object to provide deep copy and deep comparison.
class PyObject {
public:
  PyObject() = default;
  PyObject(PyObject &&other) = default;
  PyObject &operator=(PyObject &&other) = default;

  PyObject(const PyObject &other) { *this = other; }
  PyObject &operator=(const PyObject &other) {
    py::object copy = py::module::import("copy");
    py::object deepcopy = copy.attr("deepcopy");
    m_object = deepcopy(other.m_object);
    return *this;
  }

  PyObject(const py::object &object) : m_object(object) {}

  const py::object &to_pybind() const noexcept { return m_object; }
  py::object &to_pybind() noexcept { return m_object; }

  bool operator==(const PyObject &other) const {
    return to_pybind().equal(other.to_pybind());
  }

private:
  py::object m_object;
};

} // namespace scipp::python

#endif // SCIPP_PYTHON_PY_OBJECT_H
