// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "detail.h"
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

void init_histogram(py::module &m) {
  auto doc = Docstring()
                 .description(
                     "Histograms the input event data along the dimensions of "
                     "the supplied Variable describing the bin edges.")
                 .returns("Histogrammed data with units of counts.")
                 .param("x", "Input data to be histogrammed.", "DataArray")
                 .param("bins", "Bin edges.", "Variable");

  m.def("histogram",
        [](CstViewRef<DataArray> x, CstViewRef<Variable> bins) {
          return histogram(x, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        doc.rtype("DataArray").c_str());

  m.def("histogram",
        [](const Dataset &x, CstViewRef<Variable> bins) {
          return histogram(x, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        doc.rtype("Dataset")
            .param("x", "Input data to be histogrammed.", "Dataset")
            .c_str());

  doc =
      doc.clear()
          .description("Accepts realigned data and histograms the unaligned "
                       "content according to the realigning axes.")
          .returns("Histogrammed data with units of counts.")
          .param("x", "Input realigned data to be histogrammed.", "DataArray");

  m.def("histogram", [](CstViewRef<DataArray> x) { return histogram(x); },
        py::arg("x"), py::call_guard<py::gil_scoped_release>(),
        doc.rtype("DataArray").c_str());

  m.def("histogram", [](const Dataset &x) { return histogram(x); },
        py::arg("x"), py::call_guard<py::gil_scoped_release>(),
        doc.rtype("Dataset")
            .param("x", "Input realigned data to be histogrammed.", "Dataset")
            .c_str());
}
