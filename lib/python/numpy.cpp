// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "numpy.h"

#include "dtype.h"

void ElementTypeMap<scipp::core::time_point>::check_assignable(
    const nb::object &obj, const sc_units::Unit unit) {
  // Materialize as nb::object: with `auto` this would be a lazy attribute
  // accessor whose base (the temporary array) dies at the end of the
  // statement.
  const nb::object dtype =
      nb::module_::import_("numpy").attr("asarray")(obj).attr("dtype");
  if (nb::cast<std::string>(dtype.attr("kind")) == "i") {
    return; // just assume we can assign from int
  }
  const auto np_unit =
      parse_datetime_dtype(nb::cast<std::string>(dtype.attr("name")));
  if (np_unit != unit) {
    std::ostringstream oss;
    oss << "Unable to assign datetime with unit " << to_string(np_unit)
        << " to " << to_string(unit);
    throw std::invalid_argument(oss.str());
  }
}

scipp::core::time_point make_time_point(const nb::object &buffer,
                                        const int64_t scale) {
  // Cannot cast the object directly because numpy.datetime64.__int__
  // delegates to datetime.datetime if the unit is larger than ns and
  // that cannot be converted to long.
  using PyType = typename ElementTypeMap<core::time_point>::PyType;
  return core::time_point{
      nb::cast<PyType>(buffer.attr("astype")(np_dtype_of<PyType>())) * scale};
}
