// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/core/except.h"
#include "pybind11.h"
#include "scipp/dataset/except.h"

namespace py = pybind11;

void init_exceptions(py::module &m) {
  using namespace scipp;
  py::register_exception<except::UnitError>(m, "UnitError", PyExc_RuntimeError);
  py::register_exception<except::TypeError>(m, "DTypeError",
                                            PyExc_RuntimeError);
  py::register_exception<except::DimensionError>(m, "DimensionError",
                                                 PyExc_RuntimeError);
  py::register_exception<except::BinnedDataError>(m, "BinnedDataError",
                                                  PyExc_RuntimeError);
  py::register_exception<except::CoordMismatchError>(m, "CoordError",
                                                     PyExc_RuntimeError);
}
