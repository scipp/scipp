// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <string_view>

#include <scipp/core/dtype.h>
#include <scipp/units/unit.h>

#include "pybind11.h"
#include "unit.h"

namespace pybind11 {
class dtype;
}

scipp::core::DType dtype_of(const pybind11::object &x);

scipp::core::DType scipp_dtype(const pybind11::object &type);

std::tuple<scipp::core::DType, std::optional<scipp::sc_units::Unit>>
cast_dtype_and_unit(const pybind11::object &dtype, const ProtoUnit &unit);

void ensure_conversion_possible(scipp::core::DType from, scipp::core::DType to,
                                const std::string &data_name);

template <class T, class = void> struct converting_cast {
  static decltype(auto) cast(const pybind11::object &obj) {
    return obj.cast<T>();
  }
};

template <class T>
struct converting_cast<T, std::enable_if_t<std::is_integral_v<T>>> {
  static decltype(auto) cast(const pybind11::object &obj) {
    if (dtype_of(obj) == scipp::dtype<double>) {
      // This conversion is not implemented in pybind11 v2.6.2
      return obj.cast<pybind11::int_>().cast<T>();
    } else {
      // All other conversions are either supported by pybind11 or not
      // desired anyway.
      return obj.cast<T>();
    }
  }
};

scipp::core::DType
common_dtype(const pybind11::object &values, const pybind11::object &variances,
             scipp::core::DType dtype,
             scipp::core::DType default_dtype = scipp::core::dtype<double>);

bool has_datetime_dtype(const pybind11::object &obj);

[[nodiscard]] scipp::sc_units::Unit
parse_datetime_dtype(const std::string &dtype_name);
[[nodiscard]] scipp::sc_units::Unit
parse_datetime_dtype(const pybind11::object &dtype);
