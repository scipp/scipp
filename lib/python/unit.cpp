// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

#include "unit.h"

#include "scipp/units/string.h"

#include "dtype.h"

using namespace scipp;
namespace py = pybind11;

namespace {
bool temporal_or_dimensionless(const units::Unit unit) {
  return unit == units::one || unit.has_same_base(units::s);
}
} // namespace

units::Unit make_unit(const ProtoUnit &unit) {
  if (std::holds_alternative<py::none>(unit))
    return units::none;
  if (std::holds_alternative<std::string>(unit))
    return units::Unit(std::get<std::string>(unit));
  return std::get<units::Unit>(unit);
}

std::tuple<units::Unit, int64_t>
get_time_unit(const std::optional<scipp::units::Unit> value_unit,
              const std::optional<scipp::units::Unit> dtype_unit,
              const units::Unit sc_unit) {
  if (!temporal_or_dimensionless(sc_unit)) {
    throw std::invalid_argument("Invalid unit for dtype=datetime64: " +
                                to_string(sc_unit));
  }
  if (dtype_unit.value_or(units::one) != units::one &&
      (sc_unit != units::one && *dtype_unit != sc_unit)) {
    throw std::invalid_argument(
        "dtype (datetime64[" + to_string(*dtype_unit) +
        "]) has a different time unit from 'unit' argument (" +
        to_string(sc_unit) + ")");
  }
  units::Unit actual_unit = units::one;
  if (sc_unit != units::one)
    actual_unit = sc_unit;
  else if (dtype_unit.value_or(units::one) != units::one)
    actual_unit = *dtype_unit;
  else if (value_unit.has_value())
    actual_unit = *value_unit;

  // TODO implement
  if (value_unit && value_unit != actual_unit) {
    throw std::runtime_error("Conversion of time units is not implemented.");
  }

  return {actual_unit, 1};
}

std::tuple<units::Unit, int64_t> get_time_unit(const py::buffer &value,
                                               const py::object &dtype,
                                               const units::Unit unit) {
  return get_time_unit(
      value.is_none() || value.attr("dtype").attr("kind").cast<char>() != 'M'
          ? std::optional<units::Unit>{}
          : parse_datetime_dtype(value),
      dtype.is_none() ? std::optional<units::Unit>{}
                      : parse_datetime_dtype(dtype),
      unit);
}

template <>
std::tuple<scipp::units::Unit, scipp::units::Unit>
common_unit<scipp::core::time_point>(const pybind11::object &values,
                                     const scipp::units::Unit unit) {
  if (!temporal_or_dimensionless(unit)) {
    throw std::invalid_argument("Invalid unit for dtype=datetime64: " +
                                to_string(unit));
  }

  if (values.is_none() || !has_datetime_dtype(values)) {
    return std::tuple{unit, unit};
  }

  const auto value_unit = parse_datetime_dtype(values);
  if (unit == units::one) {
    return std::tuple{value_unit, value_unit};
  } else {
    return std::tuple{value_unit, unit};
  }
}

std::string to_numpy_time_string(const scipp::units::Unit unit) {
  return unit == units::us
             ? std::string("us")
             : unit == units::Unit("min") ? std::string("m") : to_string(unit);
}
