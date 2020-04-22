// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "pybind11.h"

namespace py = pybind11;

namespace scipp::python {

/// Wrapper around pybind11::object to provide deep copy and deep comparison.
///
/// Whenever this class makes calls to Python it acquires the GIL first to
/// ensure that it can be used as part of code that has a released GIL. Since
/// this class is meant as an element type in Variable, this is often the case,
/// e.g., in any operation that makes copies of variables.
class PyObject {
public:
  PyObject() = default;
  PyObject(PyObject &&other) = default;
  PyObject &operator=(PyObject &&other) = default;
  PyObject(const PyObject &other) : PyObject(other.m_object) {}
  PyObject &operator=(const PyObject &other) { return *this = PyObject(other); }
  ~PyObject();

  PyObject(const py::object &object);

  const py::object &to_pybind() const noexcept { return m_object; }
  py::object &to_pybind() noexcept { return m_object; }

  bool operator==(const PyObject &other) const;

private:
  py::object m_object;
};

} // namespace scipp::python
