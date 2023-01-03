// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include <sstream>
#include <string>

#include "pybind11.h"

namespace scipp::python {
/// Format a string from all arguments.
template <class... Args> std::string format(Args &&...args) {
  std::ostringstream oss;
  (oss << ... << std::forward<Args>(args));
  return oss.str();
}
} // namespace scipp::python
