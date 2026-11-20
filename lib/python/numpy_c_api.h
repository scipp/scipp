// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
#pragma once

#include <span>
#include <type_traits>

#include "nanobind.h"
#define PY_ARRAY_UNIQUE_SYMBOL scipp_numpy_api
#ifndef SCIPP_IMPORT_NUMPY
#define NO_IMPORT_ARRAY
#endif
#include <numpy/ndarrayobject.h>

#include "scipp/common/index.h"

namespace scipp::python {

template <class T> constexpr NPY_TYPES numpy_typenum() {
  if constexpr (std::is_same_v<T, double>)
    return NPY_DOUBLE;
  else if constexpr (std::is_same_v<T, float>)
    return NPY_FLOAT;
  else if constexpr (std::is_same_v<T, int64_t>)
    return NPY_INT64;
  else if constexpr (std::is_same_v<T, uint64_t>)
    return NPY_UINT64;
  else if constexpr (std::is_same_v<T, int32_t>)
    return NPY_INT32;
  else if constexpr (std::is_same_v<T, bool>)
    return NPY_BOOL;
  else
    static_assert(sizeof(T) == 0, "unsupported NumPy element type");
}

nanobind::object as_numpy_array(nanobind::handle obj,
                                NPY_TYPES type = NPY_NOTYPE, int flags = 0);
nanobind::object numpy_scalar(NPY_TYPES type, const void *data);
nanobind::object make_numpy_array(NPY_TYPES type, const void *data,
                                  std::span<const scipp::index> shape,
                                  std::span<const scipp::index> strides,
                                  bool writeable, nanobind::object owner);
} // namespace scipp::python
