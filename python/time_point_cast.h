// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include <iomanip>
#include <sstream>

#include <chrono>
#include <cmath>
#include <ctime>
#include <pybind11/chrono.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "scipp/core/dtype.h"
#include "scipp/core/time_point.h"
#include "scipp/units/unit.h"

namespace py = pybind11;

/// Extract the unit from a numpy datetime64 dtype name.
/// Throws if the name does not math
[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(std::string_view dtype_name);

namespace pybind11::detail {
/// Type caster: scipp::core::time_point <-> NumPy.datetime64
template <>
struct type_caster<scipp::core::time_point>
    : public type_caster_base<scipp::core::time_point> {
public:
  PYBIND11_TYPE_CASTER(scipp::core::time_point, _("numpy.datetime64"));

  // Conversion part 1 (Python -> C++)
  bool load(py::handle src, bool convert) {
    if (!convert)
      return false;
    if (!src)
      return false;

    const auto unit = parse_datetime_dtype(
        src.attr("dtype").attr("name").cast<std::string_view>());
    const auto time = src.attr("astype")("int").cast<int64_t>();

    // time_point requires ns.
    // TODO use units lib?
    const auto ns_time = unit == scipp::units::s    ? time * 1000000000
                         : unit == scipp::units::us ? time * 1000
                                                    : time;
    value = scipp::core::time_point(ns_time);
    return true;
  }

  /// Conversion part 2 (C++ -> Python)
  static py::handle cast(const scipp::core::time_point &src,
                         py::return_value_policy, py::handle var_h) {
    int64_t epoch = src.time_since_epoch();
    auto unit = var_h.attr("unit").cast<py::str>();
    py::object np = py::module::import("numpy");
    py::object nd64 = np.attr("datetime64")(epoch, unit);
    return nd64.release();
  }
};

} // namespace pybind11::detail
