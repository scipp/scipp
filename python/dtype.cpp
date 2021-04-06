// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dtype.h"

#include <regex>

#include "pybind11.h"
#include "scipp/core/string.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

void init_dtype(py::module &m) {
  py::class_<DType>(m, "_DType")
      .def(py::self == py::self)
      .def("__repr__", [](const DType self) { return to_string(self); });
  auto dtype = m.def_submodule("dtype");
  for (const auto &[key, name] : core::dtypeNameRegistry()) {
    dtype.attr(name.c_str()) = key;
  }
}

scipp::core::DType scipp_dtype(const py::dtype &type) {
  if (type.is(py::dtype::of<double>()))
    return scipp::core::dtype<double>;
  if (type.is(py::dtype::of<float>()))
    return scipp::core::dtype<float>;
  // See https://github.com/pybind/pybind11/pull/1329, int64_t not
  // matching numpy.int64 correctly.
  if (type.is(py::dtype::of<std::int64_t>()) ||
      (type.kind() == 'i' && type.itemsize() == 8))
    return scipp::core::dtype<int64_t>;
  if (type.is(py::dtype::of<std::int32_t>()) ||
      (type.kind() == 'i' && type.itemsize() == 4))
    return scipp::core::dtype<int32_t>;
  if (type.is(py::dtype::of<bool>()))
    return scipp::core::dtype<bool>;
  if (type.kind() == 'U')
    return scipp::core::dtype<std::string>;
  if (type.kind() == 'M') {
    return scipp::core::dtype<scipp::core::time_point>;
  }
  throw std::runtime_error(
      "Unsupported numpy dtype: " +
      py::str(static_cast<py::handle>(type)).cast<std::string>() +
      "\n"
      "Supported types are: bool, float32, float64,"
      " int32, int64, string, and datetime64");
}

scipp::core::DType scipp_dtype(const py::object &type) {
  // Check None first, then native scipp Dtype, then numpy.dtype
  if (type.is_none())
    return dtype<void>;
  try {
    return type.cast<DType>();
  } catch (const py::cast_error &) {
    return scipp_dtype(py::dtype::from_args(type));
  }
}

[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const std::string &dtype_name) {
  static std::regex datetime_regex{R"(datetime64(\[(\w+)\])?)"};
  constexpr size_t unit_idx = 2;
  std::smatch match;
  if (!std::regex_match(dtype_name, match, datetime_regex) ||
      match.size() != 3) {
    throw std::invalid_argument("Invalid dtype, expected datetime64, got " +
                                dtype_name);
  }

  if (match.length(unit_idx) == 0) {
    return scipp::units::dimensionless;
  } else if (match[unit_idx] == "s") {
    return scipp::units::s;
  } else if (match[unit_idx] == "ms") {
    static const auto ms = units::Unit("ms");
    return ms;
  } else if (match[unit_idx] == "us") {
    return scipp::units::us;
  } else if (match[unit_idx] == "ns") {
    return scipp::units::ns;
  }

  throw std::invalid_argument(std::string("Unsupported unit in datetime: ") +
                              std::string(match[unit_idx]));
}

[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const pybind11::object &dtype) {
  if (py::hasattr(dtype, "dtype")) {
    return parse_datetime_dtype(dtype.attr("dtype"));
  } else if (py::hasattr(dtype, "name")) {
    return parse_datetime_dtype(dtype.attr("name").cast<std::string>());
  } else {
    return parse_datetime_dtype(py::str(dtype).cast<std::string>());
  }
}
