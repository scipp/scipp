// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "pybind11.h"

#include "scipp/dataset/bins_nanmean.h"
#include "scipp/dataset/nanmean.h"
#include "scipp/variable/reduction.h"

using namespace scipp;

namespace py = pybind11;

template <typename T> void bind_bins_nanmean(py::module &m) {
  m.def(
      "bins_nanmean", [](const T &x) { return bins_nanmean(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
}

void init_bins_nanmean(py::module &m) {
  bind_bins_nanmean<Variable>(m);
  bind_bins_nanmean<DataArray>(m);
  bind_bins_nanmean<Dataset>(m);
}

void init_reduction(py::module &m) { init_bins_nanmean(m); }