// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include "scipp/dataset/counts.h"
#include "scipp/dataset/dataset.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

void init_counts(py::module &m) {
  m.def(
      "counts_to_density",
      [](const DatasetConstView &d, const Dim dim) {
        return counts::toDensity(Dataset(d), dim);
      },
      py::arg("x"), py::arg("dim"));

  m.def(
      "counts_to_density",
      [](const DataArrayConstView &d, const Dim dim) {
        return counts::toDensity(DataArray(d), dim);
      },
      py::arg("x"), py::arg("dim"));

  m.def(
      "density_to_counts",
      [](const DatasetConstView &d, const Dim dim) {
        return counts::fromDensity(Dataset(d), dim);
      },
      py::arg("x"), py::arg("dim"));

  m.def(
      "density_to_counts",
      [](const DataArrayConstView &d, const Dim dim) {
        return counts::fromDensity(DataArray(d), dim);
      },
      py::arg("x"), py::arg("dim"));
}
