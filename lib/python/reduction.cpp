// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/dataset/reduction.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> void bind_mean(py::module &m) {
  m.def(
      "mean", [](const T &x) { return mean(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "mean",
      [](const T &x, const std::string &dim) { return mean(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_mean_out(py::module &m) {
  m.def(
      "mean",
      [](const T &x, const std::string &dim, T &out) {
        return mean(x, Dim{dim}, out);
      },
      py::arg("x"), py::arg("dim"), py::kw_only(), py::arg("out"),
      py::call_guard<py::gil_scoped_release>());
}
template <class T> void bind_nanmean(py::module &m) {
  m.def(
      "nanmean", [](const T &x) { return nanmean(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "nanmean",
      [](const T &x, const std::string &dim) { return nanmean(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nanmean_out(py::module &m) {
  m.def(
      "nanmean",
      [](const T &x, const std::string &dim, T &out) {
        return mean(x, Dim{dim}, out);
      },
      py::arg("x"), py::arg("dim"), py::kw_only(), py::arg("out"),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_sum(py::module &m) {
  m.def(
      "sum", [](const T &x) { return sum(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "sum",
      [](const T &x, const std::string &dim) { return sum(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_sum_out(py::module &m) {
  m.def(
      "sum",
      [](const T &x, const std::string &dim, T &out) {
        return sum(x, Dim{dim}, out);
      },
      py::arg("x"), py::arg("dim"), py::kw_only(), py::arg("out"),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nansum(py::module &m) {
  m.def(
      "nansum", [](const T &x) { return nansum(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());

  m.def(
      "nansum",
      [](const T &x, const std::string &dim) { return nansum(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nansum_out(py::module &m) {
  m.def(
      "nansum",
      [](const T &x, const std::string &dim, T &out) {
        return nansum(x, Dim{dim}, out);
      },
      py::arg("x"), py::arg("dim"), py::kw_only(), py::arg("out"),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_min(py::module &m) {
  m.def(
      "min", [](const T &x) { return min(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "min",
      [](const T &x, const std::string &dim) { return min(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_max(py::module &m) {
  m.def(
      "max", [](const T &x) { return max(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "max",
      [](const T &x, const std::string &dim) { return max(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nanmin(py::module &m) {
  m.def(
      "nanmin", [](const T &x) { return nanmin(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "nanmin",
      [](const T &x, const std::string &dim) { return nanmin(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_nanmax(py::module &m) {
  m.def(
      "nanmax", [](const T &x) { return nanmax(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "nanmax",
      [](const T &x, const std::string &dim) { return nanmax(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_all(py::module &m) {
  m.def(
      "all", [](const T &x) { return all(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "all",
      [](const T &x, const std::string &dim) { return all(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_any(py::module &m) {
  m.def(
      "any", [](const T &x) { return any(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "any",
      [](const T &x, const std::string &dim) { return any(x, Dim{dim}); },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

void init_reduction(py::module &m) {
  bind_mean<Variable>(m);
  bind_mean<DataArray>(m);
  bind_mean<Dataset>(m);
  bind_mean_out<Variable>(m);

  bind_nanmean<Variable>(m);
  bind_nanmean<DataArray>(m);
  bind_nanmean<Dataset>(m);
  bind_nanmean_out<Variable>(m);

  bind_sum<Variable>(m);
  bind_sum<DataArray>(m);
  bind_sum<Dataset>(m);
  bind_sum_out<Variable>(m);

  bind_nansum<Variable>(m);
  bind_nansum<DataArray>(m);
  bind_nansum<Dataset>(m);
  bind_nansum_out<Variable>(m);

  bind_min<Variable>(m);
  bind_max<Variable>(m);
  bind_nanmin<Variable>(m);
  bind_nanmax<Variable>(m);
  bind_all<Variable>(m);
  bind_any<Variable>(m);
}
