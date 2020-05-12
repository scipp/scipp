// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> void bind_histogram(py::module &m) {
  auto doc = Docstring()
                 .description(
                     "Histograms the input event data along the dimensions of "
                     "the supplied Variable describing the bin edges.")
                 .returns("Histogrammed data with units of counts.")
                 .rtype<T>()
                 .template param<T>("x", "Input data to be histogrammed.")
                 .param("bins", "Bin edges.", "Variable");
  m.def(
      "histogram",
      [](const typename T::const_view_type &x,
         const typename Variable::const_view_type &bins) {
        return histogram(x, bins);
      },
      py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());

  doc = Docstring()
            .description("Accepts realigned data and histograms the unaligned "
                         "content according to the realigning axes.")
            .returns("Histogrammed data with units of counts.")
            .rtype<T>()
            .template param<T>("x", "Input realigned data to be histogrammed.");
  m.def(
      "histogram",
      [](const typename T::const_view_type &x) { return histogram(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
}

void init_histogram(py::module &m) {
  bind_histogram<DataArray>(m);
  bind_histogram<Dataset>(m);
}
