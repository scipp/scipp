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

void init_exceptions(py::module &m) {
  using namespace scipp;
  py::register_exception<except::UnitError>(m, "UnitError", PyExc_RuntimeError);

  py::register_exception<except::TypeError>(m, "DTypeError", PyExc_TypeError);
  py::register_exception<except::DimensionError>(m, "DimensionError",
                                                 PyExc_RuntimeError);
  py::register_exception<except::BinnedDataError>(m, "BinnedDataError",
                                                  PyExc_RuntimeError);
  py::register_exception<except::SizeError>(m, "SizeError", PyExc_RuntimeError);
  py::register_exception<except::SliceError>(m, "SliceError", PyExc_IndexError);
  py::register_exception<except::VariancesError>(m, "VariancesError",
                                                 PyExc_RuntimeError);
  py::register_exception<except::BinEdgeError>(m, "BinEdgeError",
                                               PyExc_RuntimeError);
  py::register_exception<except::NotFoundError>(m, "NotFoundError",
                                                PyExc_RuntimeError);

  py::register_exception<except::VariableError>(m, "VariableError",
                                                PyExc_RuntimeError);

  py::register_exception<except::DataArrayError>(m, "DataArrayError",
                                                 PyExc_RuntimeError);
  py::register_exception<except::DatasetError>(m, "DatasetError",
                                               PyExc_RuntimeError);
  py::register_exception<except::CoordMismatchError>(m, "CoordError",
                                                     PyExc_RuntimeError);
}
