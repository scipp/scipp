// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "numpy.h"

#include "dtype.h"

void ElementTypeMap<scipp::core::time_point>::check_assignable(
    const py::object &obj, const units::Unit unit) {
  const auto &dtype = obj.cast<py::array>().dtype();
  const auto np_unit =
      parse_datetime_dtype(dtype.attr("name").cast<std::string_view>());
  if (np_unit != unit) {
    std::ostringstream oss;
    oss << "Unable to assign datetime with unit " << to_string(np_unit)
        << " to " << to_string(unit);
    throw std::invalid_argument(oss.str());
  }
}
