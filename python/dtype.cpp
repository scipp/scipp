// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dtype.h"
#include "bind_enum.h"
#include "pybind11.h"
#include "scipp/core/string.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

void init_dtype(py::module &m) { bind_enum(m, "dtype", DType::Unknown); }

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
  throw std::runtime_error("Unsupported numpy dtype.");
}

// temporary solution untill https://github.com/pybind/pybind11/issues/1538
// is not solved
scipp::core::DType scipp_dtype_fall_back(const py::object &type) {
  py::object numpy = py::module::import("numpy");
  py::object maketype = numpy.attr("dtype");
  py::object new_type = maketype(type);
  return scipp_dtype(new_type.cast<py::dtype>());
}

scipp::core::DType scipp_dtype(const py::object &type) {
  // The manual conversion from py::object is solving a number of problems:
  // 1. On Travis' clang (7.0.0) we get a weird error (ImportError:
  //    UnicodeDecodeError: 'utf-8' codec can't decode byte 0xe1 in position 2:
  //    invalid continuation byte) when using the DType enum as a default value
  //    for py::arg. Importing the module fails.
  // 2. We want to support both numpy dtype as well as scipp dtype.
  // 3. In the implementation below, `type.cast<py::dtype>()` always succeeds,
  //    yielding a unsupported numpy dtype. Therefore we need to try casting to
  //    `DType` first, which works for some reason.
  if (type.is_none())
    return DType::Unknown;
  try {
    return type.cast<DType>();
  } catch (const py::cast_error &) {
    try {
      return scipp_dtype(type.cast<py::dtype>());
    } catch (...) {
      return scipp_dtype_fall_back(type);
    }
  }
}
