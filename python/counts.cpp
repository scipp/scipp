// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#include "scipp/core/counts.h"
#include "scipp/core/dataset.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

void init_counts(py::module &m) {
  m.def("counts_to_density",
        [](const DatasetConstProxy &d, const Dim dim) {
          return counts::toDensity(Dataset(d), dim);
        },
        py::arg("x"), py::arg("dim"),
        R"(
        Converts counts to count density on a given dimension.

        :param x: Data as counts.
        :param dim: Dimension on which to convert.
        :return: Data as count density.
        :rtype: Dataset)");

  m.def("counts_to_density",
        [](const DataConstProxy &d, const Dim dim) {
          return counts::toDensity(DataArray(d), dim);
        },
        py::arg("x"), py::arg("dim"),
        R"(
        Converts counts to count density on a given dimension.

        :param x: Data as counts.
        :param dim: Dimension on which to convert.
        :return: Data as count density.
        :rtype: DataArray)");

  m.def("density_to_counts",
        [](const DatasetConstProxy &d, const Dim dim) {
          return counts::fromDensity(Dataset(d), dim);
        },
        py::arg("x"), py::arg("dim"),
        R"(
        Converts count density to counts on a given dimension.

        :param x: Data as count density.
        :param dim: Dimension on which to convert.
        :return: Data as counts.
        :rtype: Dataset)");

  m.def("density_to_counts",
        [](const DataConstProxy &d, const Dim dim) {
          return counts::fromDensity(DataArray(d), dim);
        },
        py::arg("x"), py::arg("dim"),
        R"(
        Converts count density to counts on a given dimension.

        :param x: Data as count density.
        :param dim: Dimension on which to convert.
        :return: Data as counts.
        :rtype: DataArray)");
}
