// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <string_view>

#include <scipp/core/dtype.h>
#include <scipp/units/unit.h>

#include "checked_cast.h"
#include "nanobind.h"
#include "unit.h"

scipp::core::DType dtype_of(const nanobind::object &x);

scipp::core::DType scipp_dtype(const nanobind::object &type);

std::tuple<scipp::core::DType, std::optional<scipp::sc_units::Unit>>
cast_dtype_and_unit(const nanobind::object &dtype, const ProtoUnit &unit);

void ensure_conversion_possible(scipp::core::DType from, scipp::core::DType to,
                                const std::string &data_name);

template <class T, class = void> struct converting_cast {
  static decltype(auto) cast(const nanobind::object &obj) {
    return scipp::python::checked_cast<T>(obj);
  }
};

template <class T>
struct converting_cast<
    T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
  static decltype(auto) cast(const nanobind::object &obj) {
    if (dtype_of(obj) == scipp::dtype<double>) {
      // This conversion is not implemented in pybind11 v2.6.2
      return scipp::python::checked_cast<T>(nanobind::int_(obj));
    } else {
      // All other conversions are either supported by pybind11 or not
      // desired anyway.
      return scipp::python::checked_cast<T>(obj);
    }
  }
};

template <> struct converting_cast<bool> {
  static bool cast(const nanobind::object &obj) {
    // Use the truth value, as NumPy does, instead of truncating floats.
    const int result = PyObject_IsTrue(obj.ptr());
    if (result == -1)
      throw nanobind::python_error();
    return result != 0;
  }
};

scipp::core::DType
common_dtype(const nanobind::object &values, const nanobind::object &variances,
             scipp::core::DType dtype,
             scipp::core::DType default_dtype = scipp::core::dtype<double>);

bool has_datetime_dtype(const nanobind::object &obj);

[[nodiscard]] scipp::sc_units::Unit
parse_datetime_dtype(const std::string &dtype_name);
[[nodiscard]] scipp::sc_units::Unit
parse_datetime_dtype(const nanobind::object &dtype);
