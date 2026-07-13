// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

// When a module is split into several compilation units, *all* compilation
// units must include the extra headers with type casters, otherwise we get ODR
// errors/warning. This header provides all nanobind includes that we are
// using.

#include <nanobind/nanobind.h>

#include <nanobind/make_iterator.h>
#include <nanobind/ndarray.h>
#include <nanobind/operators.h>
#include <nanobind/typing.h>

// Warnings are raised by eigen headers with gcc12
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include <nanobind/eigen/dense.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

/// True if the object supports the CPython buffer protocol. Replacement for
/// pybind11's `py::isinstance<py::buffer>`: matches numpy arrays and numpy
/// scalars but not Python lists or scalars.
inline bool is_buffer_like(const nb::handle &obj) {
  return PyObject_CheckBuffer(obj.ptr()) != 0;
}

/// Convert to a numpy array via numpy.asarray, i.e. with all of numpy's
/// automatic conversions such as integer to double and packing of nested
/// sequences. Pass a numpy dtype object as `dtype` to force an element type.
template <class... Dtype>
nb::object np_asarray(const nb::object &obj, const Dtype &...dtype) {
  static_assert(sizeof...(Dtype) <= 1);
  static const nb::handle asarray =
      nb::object(nb::module_::import_("numpy").attr("asarray")).release();
  return asarray(obj, dtype...);
}

/// Convert a mapping or an iterable of key-value pairs to a Python dict,
/// like Python's `dict(obj)`. None yields an empty dict.
inline nb::dict as_pydict(const nb::object &obj) {
  if (obj.is_none())
    return {};
  PyObject *d = PyObject_CallOneArg(reinterpret_cast<PyObject *>(&PyDict_Type),
                                    obj.ptr());
  if (d == nullptr)
    throw nb::python_error();
  return nb::steal<nb::dict>(d);
}
