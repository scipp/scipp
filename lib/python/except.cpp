// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/core/except.h"
#include "pybind11.h"
#include "scipp/dataset/except.h"
#include "scipp/units/except.h"
#include "scipp/variable/except.h"

namespace py = pybind11;

namespace {
/// Translate an exception into a standard Python exception.
template <class CppException, class PyException>
void register_with_builtin_exception(const PyException &py_exception) {
  // Pybind11 does not like it when the lambda captures something,
  // so 'pass' py_exception via a static variable.
  static auto pyexc = py_exception;
  py::register_exception_translator([](std::exception_ptr p) {
    try {
      if (p)
        std::rethrow_exception(p);
    } catch (const CppException &e) {
      PyErr_SetString(pyexc, e.what());
    }
  });
}
} // namespace

void init_exceptions(py::module &m) {
  using namespace scipp;

  py::register_exception<except::BinEdgeError>(m, "BinEdgeError",
                                               PyExc_RuntimeError);
  py::register_exception<except::BinnedDataError>(m, "BinnedDataError",
                                                  PyExc_RuntimeError);
  py::register_exception<except::CoordMismatchError>(m, "CoordError",
                                                     PyExc_RuntimeError);
  py::register_exception<except::DataArrayError>(m, "DataArrayError",
                                                 PyExc_RuntimeError);
  py::register_exception<except::DatasetError>(m, "DatasetError",
                                               PyExc_RuntimeError);
  py::register_exception<except::DimensionError>(m, "DimensionError",
                                                 PyExc_RuntimeError);
  py::register_exception<except::TypeError>(m, "DTypeError", PyExc_TypeError);
  py::register_exception<except::UnitError>(m, "UnitError", PyExc_RuntimeError);
  py::register_exception<except::VariableError>(m, "VariableError",
                                                PyExc_RuntimeError);
  py::register_exception<except::VariancesError>(m, "VariancesError",
                                                 PyExc_RuntimeError);

  register_with_builtin_exception<except::SizeError>(PyExc_ValueError);
  register_with_builtin_exception<except::SliceError>(PyExc_IndexError);
  register_with_builtin_exception<except::NotFoundError>(PyExc_KeyError);
  register_with_builtin_exception<except::NotImplementedError>(
      PyExc_NotImplementedError);
}
