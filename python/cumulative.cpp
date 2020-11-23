// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/variable/cumulative.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> void bind_cumsum(py::module &m) {
  m.def(
      "cumsum",
      [](const typename T::const_view_type &a, const bool inclusive) {
        return cumsum(a, inclusive);
      },
      py::arg("a"), py::arg("inclusive") = true,
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "cumsum",
      [](const typename T::const_view_type &a, const Dim dim,
         const bool inclusive) { return cumsum(a, dim, inclusive); },
      py::arg("a"), py::arg("dim"), py::arg("inclusive") = true,
      py::call_guard<py::gil_scoped_release>());
}

void init_cumulative(py::module &m) { bind_cumsum<Variable>(m); }
