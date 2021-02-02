// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/comparison.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> void bind_is_approx(py::module &m) {
  m.def(
      "is_approx",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y,
         const typename T::const_view_type &tol) {
        return is_approx(x, y, tol);
      },
      py::arg("x"), py::arg("y"), py::arg("tol"),
      py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_is_equal(py::module &m) {
  m.def(
      "is_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return x == y; },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

void init_comparison(py::module &m) {
  bind_is_approx<Variable>(m);
  bind_is_equal<Variable>(m);
  bind_is_equal<Dataset>(m);
  bind_is_equal<DataArray>(m);
}
