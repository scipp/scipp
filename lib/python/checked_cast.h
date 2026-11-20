// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
#pragma once

#include <type_traits>

#include "nanobind.h"

namespace scipp::python {

/// Cast like pybind11's `obj.cast<T>()`. A failed nanobind cast only reports
/// std::bad_cast, so raise RuntimeError with the types involved. None for a
/// bound C++ type was an incompatible argument (TypeError) in pybind11.
template <class T> T checked_cast(nanobind::handle obj) {
  try {
    return nanobind::cast<T>(obj);
  } catch (const nanobind::cast_error &) {
    const auto type = nanobind::type<std::remove_cvref_t<T>>();
    PyErr_Format(
        obj.is_none() && type.is_valid() ? PyExc_TypeError : PyExc_RuntimeError,
        "Unable to cast Python instance of type %S to C++ type '%s'",
        obj.type().ptr(),
        type.is_valid() ? nanobind::type_name(type).c_str()
                        : nanobind::detail::make_caster<T>::Name.text);
    throw nanobind::python_error();
  }
}

/// Like pybind11's dict(obj): accepts mappings and iterables of pairs.
inline nanobind::dict to_pydict(nanobind::handle obj) {
  if (nanobind::isinstance<nanobind::dict>(obj))
    return nanobind::borrow<nanobind::dict>(obj);
  const nanobind::handle dict_type(
      reinterpret_cast<::PyObject *>(&PyDict_Type));
  return nanobind::cast<nanobind::dict>(dict_type(obj));
}

} // namespace scipp::python
