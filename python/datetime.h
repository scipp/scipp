// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include <iomanip>
#include <sstream>

#include <chrono>
#include <cmath>
#include <ctime>
#include <datetime.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "scipp/core/dtype.h"

namespace py = pybind11;

// type caster: scipp::core::time_point <-> NumPy-datetime64
namespace pybind11 {
namespace detail {
template <> struct type_caster<scipp::core::time_point> {
public:
  PYBIND11_TYPE_CASTER(scipp::core::time_point, _("numpy.datetime64"));

  // Conversion part 1 (Python -> C++)
  bool load(py::handle src, bool convert) {
    if (!convert)
      return false;

    if (!src)
      return false;
    // Get timestamp (ns precision) from the numpy object
    PyObject *source = src.ptr();
    PyObject *tmp = PyNumber_Long(source);
    long timestamp = PyLong_AsLong(tmp);
    value = scipp::core::time_point(timestamp);

    return true;
  }

  // Conversion part 2 (C++ -> Python)
  static py::handle cast(const scipp::core::time_point &src,
                         py::return_value_policy, py::handle) {
    auto epoch = src.time_since_epoch();
    std::chrono::system_clock::time_point tp{std::chrono::nanoseconds{epoch}};
    auto time = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::gmtime(&time);

    // GMTime has only ms resolution, so we need to add ns explicitly
    auto ns = epoch % 1000000000;
    std::stringstream ss;
    ss << std::put_time(&tm, "%FT%T.") << ns;
    return PyUnicode_FromString(c_str(ss.str()));
  }
};
} // namespace detail
} // namespace pybind11
