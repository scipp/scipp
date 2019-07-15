// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef PYTHON_TO_STRING_H
#define PYTHON_TO_STRING_H

#include <regex>
#include <string>

#include "scipp/core/except.h"

namespace scipp::python {
template <class T> std::string to_string(const T &object) {
  return std::regex_replace(scipp::core::to_string(object), std::regex("::"),
                            ".");
}
} // namespace scipp::python

#endif // PYTHON_TO_STRING_H
