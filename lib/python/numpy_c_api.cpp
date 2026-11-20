// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
#include "numpy_c_api.h"

#include "scipp/core/sizes.h"
#include <boost/container/small_vector.hpp>

namespace nb = nanobind;

namespace scipp::python {
namespace {
PyArray_Descr *descriptor(const NPY_TYPES type) {
  auto *descr = PyArray_DescrFromType(type);
  if (!descr)
    throw nb::python_error();
  return descr;
}

nb::object checked(::PyObject *result) {
  if (!result)
    throw nb::python_error();
  return nb::steal(result);
}
} // namespace

nb::object as_numpy_array(nb::handle obj, const NPY_TYPES type,
                          const int flags) {
  // FromAny steals the descriptor. A null descriptor lets NumPy infer it.
  return checked(PyArray_FromAny(
      obj.ptr(), type == NPY_NOTYPE ? nullptr : descriptor(type), 0, 0,
      NPY_ARRAY_ENSUREARRAY | NPY_ARRAY_FORCECAST | flags, nullptr));
}

nb::object numpy_scalar(const NPY_TYPES type, const void *data) {
  auto *descr = descriptor(type);
  auto *scalar = PyArray_Scalar(const_cast<void *>(data), descr, nullptr);
  Py_DECREF(descr);
  return checked(scalar);
}

nb::object make_numpy_array(const NPY_TYPES type, const void *data,
                            const std::span<const scipp::index> shape,
                            const std::span<const scipp::index> strides,
                            const bool writeable, nb::object owner) {
  using Indices = boost::container::small_vector<npy_intp, core::NDIM_STACK>;
  const Indices dims(shape.begin(), shape.end());
  Indices byte_strides;
  byte_strides.reserve(strides.size());
  auto *descr = descriptor(type);
  for (const auto stride : strides)
    byte_strides.push_back(stride * PyDataType_ELSIZE(descr));
  // NewFromDescr steals the descriptor and SetBaseObject steals the owner.
  auto array = checked(PyArray_NewFromDescr(
      &PyArray_Type, descr, static_cast<int>(dims.size()), dims.data(),
      byte_strides.data(), const_cast<void *>(data),
      writeable ? NPY_ARRAY_WRITEABLE : 0, nullptr));
  auto *view = reinterpret_cast<PyArrayObject *>(array.ptr());
  // NumPy allocates writable storage when an empty Scipp buffer is null.
  if (!writeable)
    PyArray_CLEARFLAGS(view, NPY_ARRAY_WRITEABLE);
  if (PyArray_SetBaseObject(view, owner.release().ptr()) < 0)
    throw nb::python_error();
  return array;
}
} // namespace scipp::python
