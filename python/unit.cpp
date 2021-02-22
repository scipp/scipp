// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

#include "unit.h"

#include "scipp/units/string.h"

#include "dtype.h"

using namespace scipp;
namespace py = pybind11;

namespace {
// TODO proper dimension check
bool temporal_or_dimensionless(const units::Unit unit) {
  static const auto ms = units::Unit("ms");
  return unit != units::one && unit != units::s && unit != units::ns &&
         unit != units::us && unit != ms;
}
} // namespace

std::tuple<units::Unit, int64_t>
get_time_unit(const std::optional<scipp::units::Unit> value_unit,
              const std::optional<scipp::units::Unit> dtype_unit,
              const units::Unit sc_unit) {
  if (temporal_or_dimensionless(sc_unit)) {
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
  else
    throw std::invalid_argument("Unable to infer time unit from any argument.");

  // TODO implement
  if (value_unit && value_unit != actual_unit) {
    throw std::runtime_error("Conversion of time units is not implemented.");
  }

  return {actual_unit, 1};
}

std::tuple<units::Unit, int64_t> get_time_unit(const py::buffer &value,
                                               const py::object &dtype,
                                               const units::Unit unit) {
  return get_time_unit(value.is_none() ? std::optional<units::Unit>{}
                                       : parse_datetime_dtype(value),
                       dtype.is_none()
                           ? std::optional<units::Unit>{}
                           : parse_datetime_dtype(py::dtype::from_args(dtype)),
                       unit);
}

std::string to_string_ascii_time(const scipp::units::Unit unit) {
  return unit == units::us ? std::string("us") : to_string(unit);
}