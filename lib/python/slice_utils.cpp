// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "nanobind.h"

#include "scipp/variable/variable.h"

namespace nb = nanobind;
using namespace scipp;
using namespace scipp::variable;

std::tuple<Variable, Variable>
label_bounds_from_pyslice(const nb::slice &py_slice) {
  auto start = nb::getattr(py_slice, "start");
  auto stop = nb::getattr(py_slice, "stop");
  auto step = nb::getattr(py_slice, "step");
  auto start_var = start.is_none() ? Variable{} : nb::cast<Variable>(start);
  auto stop_var = stop.is_none() ? Variable{} : nb::cast<Variable>(stop);
  if (!step.is_none()) {
    throw std::runtime_error(
        "Step cannot be specified for value based slicing.");
  }
  return std::tuple{start_var, stop_var};
}
