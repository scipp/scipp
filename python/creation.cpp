// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/creation.h"

#include "dtype.h"
#include "pybind11.h"

using namespace scipp;

namespace py = pybind11;

void init_creation(py::module &m) {
  m.def("empty", [](const std::vector<Dim> &dims,
                    const std::vector<scipp::index> &shape,
                    const units::Unit &unit, const py::object &dtype,
                    const bool variances) {
    const auto dtype_ = scipp_dtype(dtype);
    py::gil_scoped_release release;
    return variable::empty(Dimensions(dims, shape), unit, dtype_, variances);
  });
  m.def("ones", [](const std::vector<Dim> &dims,
                   const std::vector<scipp::index> &shape,
                   const units::Unit &unit, const py::object &dtype,
                   const bool variances) {
    const auto dtype_ = scipp_dtype(dtype);
    py::gil_scoped_release release;
    return variable::ones(Dimensions(dims, shape), unit, dtype_, variances);
  });
}
