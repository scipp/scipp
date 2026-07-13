// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <string_view>

#include <scipp/core/dtype.h>
#include <scipp/units/unit.h>

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
    return nb::cast<T>(obj);
  }
};

template <class T>
struct converting_cast<T, std::enable_if_t<std::is_integral_v<T>>> {
  static decltype(auto) cast(const nanobind::object &obj) {
    if constexpr (std::is_same_v<T, bool>) {
      // nanobind's bool caster does not convert from other types, pybind11's
      // did. Mirror pybind11's rule: only types implementing __bool__ via the
      // nb_bool slot (bool, np.bool_, int, float, 0-d arrays) convert; types
      // whose truthiness comes from len() (str, list, ...) are rejected
      // instead of silently mapping to True/False.
      const auto *as_number = Py_TYPE(obj.ptr())->tp_as_number;
      if (as_number == nullptr || as_number->nb_bool == nullptr)
        throw std::invalid_argument(
            "Cannot convert " +
            nb::cast<std::string>(obj.type().attr("__name__")) + " to bool.");
      return static_cast<bool>(nb::bool_(obj));
    } else if (dtype_of(obj) == scipp::dtype<double>) {
      // Explicit conversion because nb::cast does not convert
      // floating point numbers to integers.
      return nb::cast<T>(nb::int_(obj));
    } else {
      // All other conversions are either supported by nanobind or not
      // desired anyway.
      return nb::cast<T>(obj);
    }
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
