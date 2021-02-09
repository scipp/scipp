// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

#include "unit.h"

#include "scipp/units/string.h"

#include "dtype.h"

using namespace scipp;
namespace py = pybind11;

std::tuple<units::Unit, int64_t, int64_t>
get_time_unit(const std::optional<scipp::units::Unit> value_unit,
              const std::optional<scipp::units::Unit> variance_unit,
              const std::optional<scipp::units::Unit> dtype_unit,
              const units::Unit sc_unit) {
  // TODO proper dimension check
  if (sc_unit != units::one && sc_unit != units::s && sc_unit != units::ns &&
      sc_unit != units::us) {
    throw std::invalid_argument("Invalid unit for dtype=datetime64: " +
                                to_string(sc_unit));
  }
  if (dtype_unit && (sc_unit != units::one && *dtype_unit != sc_unit)) {
    throw std::invalid_argument(
        "dtype (" + to_string(*dtype_unit) +
        ") has a different time unit from 'unit' argument (" +
        to_string(sc_unit) + ")");
  }
  units::Unit actual_unit;
  if (sc_unit != units::one)
    actual_unit = sc_unit;
  else if (dtype_unit.has_value())
    actual_unit = *dtype_unit;
  else if (value_unit.has_value())
    actual_unit = *value_unit;
  else if (variance_unit.has_value())
    actual_unit = *variance_unit;
  else
    throw std::invalid_argument("Unable to infer time unit from any argument.");

  // TODO implement
  if (value_unit && value_unit != actual_unit) {
    throw std::runtime_error("Conversion of time units is not implemented.");
  }
  if (variance_unit && variance_unit != actual_unit) {
    throw std::runtime_error("Conversion of time units is not implemented.");
  }

  return {actual_unit, 1, 1};
}

std::tuple<units::Unit, int64_t, int64_t>
get_time_unit(py::buffer &value, const std::optional<py::buffer> &variance,
              const units::Unit &unit, py::object &dtype) {
  return get_time_unit(parse_datetime_dtype(value),
                       variance.has_value() ? parse_datetime_dtype(*variance)
                                            : std::optional<units::Unit>{},
                       dtype.is_none()
                           ? std::optional<units::Unit>{}
                           : parse_datetime_dtype(py::dtype::from_args(dtype)),
                       unit);
}