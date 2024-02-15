// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "pybind11.h"

#include "scipp/variable/variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::variable;

std::tuple<Variable, Variable>
label_bounds_from_pyslice(const py::slice &py_slice) {
  auto start = py::getattr(py_slice, "start");
  auto stop = py::getattr(py_slice, "stop");
  auto step = py::getattr(py_slice, "step");
  auto start_var = start.is_none() ? Variable{} : start.cast<Variable>();
  auto stop_var = stop.is_none() ? Variable{} : stop.cast<Variable>();
  if (!step.is_none()) {
    throw std::runtime_error(
        "Step cannot be specified for value based slicing.");
  }
  return std::tuple{start_var, stop_var};
}
