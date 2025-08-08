// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "numpy.h"

#include "dtype.h"

void ElementTypeMap<scipp::core::time_point>::check_assignable(
    const py::object &obj, const sc_units::Unit unit) {
  const auto &dtype = obj.cast<py::array>().dtype();
  if (dtype.attr("kind").cast<char>() == 'i') {
    return; // just assume we can assign from int
  }
  const auto np_unit =
      parse_datetime_dtype(dtype.attr("name").cast<std::string>());
  if (np_unit != unit) {
    std::ostringstream oss;
    oss << "Unable to assign datetime with unit " << to_string(np_unit)
        << " to " << to_string(unit);
    throw std::invalid_argument(oss.str());
  }
}

scipp::core::time_point make_time_point(const pybind11::buffer &buffer,
                                        const int64_t scale) {
  // buffer.cast does not always work because numpy.datetime64.__int__
  // delegates to datetime.datetime if the unit is larger than ns and
  // that cannot be converted to long.
  using PyType = typename ElementTypeMap<core::time_point>::PyType;
  return core::time_point{
      buffer.attr("astype")(py::dtype::of<PyType>()).cast<PyType>() * scale};
}
