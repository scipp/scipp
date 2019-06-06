// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPPY_DTYPE_H
#define SCIPPY_DTYPE_H

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "scipp/core/dtype.h"

namespace scippy {
class DType {
public:
  DType(const scipp::core::DType type) : m_dtype(type) {}
  DType(const pybind11::dtype &type)
      : m_dtype([](const auto &type) {
          namespace py = pybind11;
          if (type.is(py::dtype::of<double>()))
            return scipp::core::dtype<double>;
          if (type.is(py::dtype::of<float>()))
            return scipp::core::dtype<float>;
          // See https://github.com/pybind/pybind11/pull/1329, int64_t not
          // matching numpy.int64 correctly.
          if (type.is(py::dtype::of<std::int64_t>()) ||
              (type.kind() == 'i' && type.itemsize() == 8))
            return scipp::core::dtype<int64_t>;
          if (type.is(py::dtype::of<int32_t>()))
            return scipp::core::dtype<int32_t>;
          if (type.is(py::dtype::of<bool>()))
            return scipp::core::dtype<bool>;
          throw std::runtime_error("Unsupported numpy dtype.");
        }(type)) {}

private:
  scipp::core::DType m_dtype;
};
} // namespace scippy

#endif // SCIPPY_DTYPE_H
