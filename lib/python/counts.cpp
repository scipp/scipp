// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
#include "scipp/dataset/counts.h"
#include "scipp/dataset/dataset.h"

#include "nanobind.h"

using namespace scipp;
using namespace scipp::dataset;

namespace nb = nanobind;

void init_counts(nb::module_ &m) {
  m.def(
      "counts_to_density",
      [](const Dataset &d, const std::string &dim) {
        return counts::toDensity(d, Dim{dim});
      },
      nb::arg("x"), nb::arg("dim"));

  m.def(
      "counts_to_density",
      [](const DataArray &d, const std::string &dim) {
        return counts::toDensity(d, Dim{dim});
      },
      nb::arg("x"), nb::arg("dim"));

  m.def(
      "density_to_counts",
      [](const Dataset &d, const std::string &dim) {
        return counts::fromDensity(d, Dim{dim});
      },
      nb::arg("x"), nb::arg("dim"));

  m.def(
      "density_to_counts",
      [](const DataArray &d, const std::string &dim) {
        return counts::fromDensity(d, Dim{dim});
      },
      nb::arg("x"), nb::arg("dim"));
}
