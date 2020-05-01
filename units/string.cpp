// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/units/string.h"
#include <sstream>

namespace scipp::units {

std::ostream &operator<<(std::ostream &os, const Dim dim) {
  return os << to_string(dim);
}

std::string to_string(const units::Unit &unit) { return unit.name(); }

template <class T> std::string to_string(const std::initializer_list<T> items) {
  std::stringstream ss;

  for (const auto &item : items) {
    ss << to_string(item) << " ";
  }

  return ss.str();
}

template SCIPP_UNITS_EXPORT std::string to_string<scipp::units::Unit>(
    const std::initializer_list<scipp::units::Unit> items);

} // namespace scipp::units
