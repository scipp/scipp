// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dtype.h"
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
  for (const auto &[key, name] : core::dtypeNameRegistry())
    dtype.attr(name.c_str()) = key;
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
  auto type_string = py::str(static_cast<py::handle>(type));
  if (type_string.contains("<U")) // TODO is where normal way to do that ?
    return scipp::core::dtype<std::string>;
  if (type_string.contains("datetime64")) {
    return scipp::core::dtype<scipp::core::time_point>;
  }
  throw std::runtime_error("Unsupported numpy dtype.\n"
                           "Supported types are: bool, float32, float64,"
                           " int32, int64, string and datetime64");
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

// Awful handwritten parser. But std::regex does not support string_view and
// you have to go through c-strings in order to extract the unit scale.
[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const std::string_view dtype_name) {
  if (const auto length = std::size(dtype_name); length == 13 || length == 14) {
    if (dtype_name.substr(0, 11) == "datetime64[" &&
        dtype_name[length - 2] == 's' && dtype_name.back() == ']') {
      if (length == 13) {
        // no scale given
        return scipp::units::s;
      }
      switch (dtype_name[11]) {
      case 'n':
        return scipp::units::ns;
      case 'u':
        return scipp::units::us;
      default:
        throw std::invalid_argument(
            std::string("Unsupported unit in datetime: ") + dtype_name[11] +
            "s");
      }
    }
  }

  throw std::invalid_argument(
      std::string("Invalid dtype, expected datetime64, got ").append(dtype_name));
}