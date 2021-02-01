// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "time_point_cast.h"

// Awful handwritten parser. But std::regex does not support string_view and
// you have to go through c-strings in order to extract the unit scale.
[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const std::string_view dtype_name) {
  if (const auto length = size(dtype_name); length == 13 || length == 14) {
    if (dtype_name.substr(0, 11) == "datetime64[" &&
        dtype_name[length - 2] == 's' && dtype_name.back() == ']') {
      if (length == 13) {
        // no scale given
        return scipp::units::s;
      }
      switch (dtype_name[11]) {
      case 'n':
        return scipp::units::ns;
      case 'u':
        return scipp::units::us;
      default:
        throw std::invalid_argument(
            std::string("Unsupported unit in datetime: ") + dtype_name[11] +
            "s");
      }
    }
  }

  throw std::invalid_argument(
      std::string("Invalid dtype for datetime: ").append(dtype_name));
}