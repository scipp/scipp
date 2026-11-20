// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "numpy.h"
#include <sstream>

#include "checked_cast.h"
#include "dtype.h"

void ElementTypeMap<scipp::core::time_point>::check_assignable(
    const nb::object &obj, const sc_units::Unit unit) {
  // nb::object (not auto) is essential: auto would deduce a lazy attribute
  // accessor that borrows the temporary array, which dies at the end of
  // this statement.
  const nb::object dtype = scipp::python::as_numpy_array(obj).attr("dtype");
  if (nb::cast<char>(dtype.attr("kind")) == 'i') {
    return; // just assume we can assign from int
  }
  const auto np_unit =
      parse_datetime_dtype(nb::cast<std::string>(dtype.attr("name")));
  if (np_unit != unit) {
    std::ostringstream oss;
    oss << "Unable to assign datetime with unit " << to_string(np_unit)
        << " to " << to_string(unit);
    throw std::invalid_argument(oss.str());
  }
}

scipp::core::time_point make_time_point(const nb::object &buffer,
                                        const int64_t scale) {
  // pybind11 required an object implementing the buffer protocol here.
  if (!PyObject_CheckBuffer(buffer.ptr())) {
    PyErr_Format(PyExc_RuntimeError,
                 "Unable to cast Python instance of type %S to C++ type "
                 "'buffer'",
                 buffer.type().ptr());
    throw nb::python_error();
  }
  // buffer.cast does not always work because numpy.datetime64.__int__
  // delegates to datetime.datetime if the unit is larger than ns and
  // that cannot be converted to long.
  using PyType = typename ElementTypeMap<core::time_point>::PyType;
  return core::time_point{
      scipp::python::checked_cast<PyType>(buffer.attr("astype")("int64")) *
      scale};
}
