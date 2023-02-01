// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include <sstream>
#include <string>

#include "scipp/variable/variable.h"

#include "pybind11.h"

namespace scipp::python {
/// Format a string from all arguments.
template <class... Args> std::string format(Args &&...args) {
  std::ostringstream oss;
  (oss << ... << std::forward<Args>(args));
  return oss.str();
}
} // namespace scipp::python

void bind_format_variable(
    pybind11::class_<scipp::variable::Variable> &variable);
