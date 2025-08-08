// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/units/string.h"
#include <sstream>

namespace scipp::sc_units {

std::ostream &operator<<(std::ostream &os, const Dim dim) {
  return os << to_string(dim);
}

std::ostream &operator<<(std::ostream &os, const Unit unit) {
  return os << to_string(unit);
}

std::string to_string(const sc_units::Unit &unit) { return unit.name(); }

template <class T> std::string to_string(const std::initializer_list<T> items) {
  std::stringstream ss;

  for (const auto &item : items) {
    ss << to_string(item) << " ";
  }

  return ss.str();
}

template SCIPP_UNITS_EXPORT std::string to_string<scipp::sc_units::Unit>(
    const std::initializer_list<scipp::sc_units::Unit> items);

} // namespace scipp::sc_units
