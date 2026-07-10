// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "nanobind.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace nb = nanobind;

template <class T> void bind_histogram(nb::module_ &m) {
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
      [](const T &x, const Variable &bins) { return histogram(x, bins); },
      nb::arg("x"), nb::arg("bins"), nb::call_guard<nb::gil_scoped_release>(),
      doc.c_str());
}

void init_histogram(nb::module_ &m) {
  bind_histogram<DataArray>(m);
  bind_histogram<Dataset>(m);
}
