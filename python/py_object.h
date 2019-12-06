// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_PYTHON_PY_OBJECT_H
#define SCIPP_PYTHON_PY_OBJECT_H

#include "pybind11.h"

namespace py = pybind11;

namespace scipp::python {

/// Wrapper around pybind11::object to provide deep comparison.
///
/// Note that since we have no explicit copy constructor the compiler-generated
/// one makes a shallow copy, since this is what py::object does.
class PyObject {
public:
  PyObject() = default;
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
