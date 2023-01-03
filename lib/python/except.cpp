// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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

template <class CppException>
void register_exception(py::handle scope, const char *name, py::handle base,
                        const char *doc) {
  auto exc = py::register_exception<CppException>(scope, name, base);
  exc.doc() = doc;
}
} // namespace

void init_exceptions(py::module &m) {
  using namespace scipp;

  register_exception<except::BinEdgeError>(
      m, "BinEdgeError", PyExc_RuntimeError,
      "Inappropriate bin-edge coordinate.");
  register_exception<except::BinnedDataError>(m, "BinnedDataError",
                                              PyExc_RuntimeError,
                                              "Incorrect use of binned data.");
  register_exception<except::CoordMismatchError>(
      m, "CoordError", PyExc_RuntimeError,
      "Bad coordinate values or mismatching coordinates.");
  register_exception<except::DataArrayError>(
      m, "DataArrayError", PyExc_RuntimeError,
      "Incorrect use of scipp.DataArray.");
  register_exception<except::DatasetError>(
      m, "DatasetError", PyExc_RuntimeError, "Incorrect use of scipp.Dataset.");
  register_exception<except::DimensionError>(
      m, "DimensionError", PyExc_RuntimeError,
      "Inappropriate dimension labels and/or shape.");
  register_exception<except::TypeError>(m, "DTypeError", PyExc_TypeError,
                                        "Inappropriate dtype.");
  register_exception<except::UnitError>(m, "UnitError", PyExc_RuntimeError,
                                        "Inappropriate unit.");
  register_exception<except::VariableError>(m, "VariableError",
                                            PyExc_RuntimeError,
                                            "Incorrect use of scipp.Variable.");
  register_exception<except::VariancesError>(
      m, "VariancesError", PyExc_RuntimeError,
      "Variances used where they are not supported or not used where they are "
      "required.");

  register_with_builtin_exception<except::SizeError>(PyExc_ValueError);
  register_with_builtin_exception<except::SliceError>(PyExc_IndexError);
  register_with_builtin_exception<except::NotFoundError>(PyExc_KeyError);
  register_with_builtin_exception<except::NotImplementedError>(
      PyExc_NotImplementedError);
}
