// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/dataset/reduction.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> void bind_flatten(py::module &m) {
  m.def("flatten",
        py::overload_cast<const typename T::const_view_type &, const Dim>(
            &flatten),
        py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_mean(py::module &m) {
  m.def(
      "mean",
      [](const typename T::const_view_type &x, const Dim dim) {
        return mean(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_mean_out(py::module &m) {
  m.def(
      "mean",
      [](const typename T::const_view_type &x, const Dim dim,
         typename T::view_type out) { return mean(x, dim, out); },
      py::arg("x"), py::arg("dim"), py::arg("out"), py::keep_alive<0, 3>(),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_sum(py::module &m) {
  m.def(
      "sum",
      [](const typename T::const_view_type &x, const Dim dim) {
        return sum(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_sum_out(py::module &m) {
  m.def(
      "sum",
      [](const typename T::const_view_type &x, const Dim dim,
         const typename T::view_type &out) { return sum(x, dim, out); },
      py::arg("x"), py::arg("dim"), py::arg("out"), py::keep_alive<0, 3>(),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_min(py::module &m) {
  m.def(
      "min", [](const typename T::const_view_type &x) { return min(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "min",
      [](const typename T::const_view_type &x, const Dim dim) {
        return min(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_max(py::module &m) {
  m.def(
      "max", [](const typename T::const_view_type &x) { return max(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "max",
      [](const typename T::const_view_type &x, const Dim dim) {
        return max(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nanmin(py::module &m) {
  m.def(
      "nanmin", [](const typename T::const_view_type &x) { return nanmin(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "nanmin",
      [](const typename T::const_view_type &x, const Dim dim) {
        return nanmin(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nanmax(py::module &m) {
  m.def(
      "nanmax", [](const typename T::const_view_type &x) { return nanmax(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "nanmax",
      [](const typename T::const_view_type &x, const Dim dim) {
        return nanmax(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_all(py::module &m) {
  m.def(
      "all", [](const typename T::const_view_type &x) { return all(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "all",
      [](const typename T::const_view_type &x, const Dim dim) {
        return all(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_any(py::module &m) {
  m.def(
      "any", [](const typename T::const_view_type &x) { return any(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "any",
      [](const typename T::const_view_type &x, const Dim dim) {
        return any(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

void init_reduction(py::module &m) {
  bind_flatten<Variable>(m);
  bind_flatten<DataArray>(m);
  bind_flatten<Dataset>(m);

  bind_mean<Variable>(m);
  bind_mean<DataArray>(m);
  bind_mean<Dataset>(m);
  bind_mean_out<Variable>(m);

  bind_sum<Variable>(m);
  bind_sum<DataArray>(m);
  bind_sum<Dataset>(m);
  bind_sum_out<Variable>(m);

  bind_min<Variable>(m);
  bind_max<Variable>(m);
  bind_nanmin<Variable>(m);
  bind_nanmax<Variable>(m);
  bind_all<Variable>(m);
  bind_any<Variable>(m);
}
