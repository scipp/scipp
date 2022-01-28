// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/sort.h"
#include "scipp/variable/math.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/slice.h"
#include "scipp/variable/sort.h"
#include "scipp/variable/util.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

auto get_sort_order(const std::string &order) {
  if (order == "ascending")
    return SortOrder::Ascending;
  else if (order == "descending")
    return SortOrder::Descending;
  else
    throw std::runtime_error("Sort order must be 'ascending' or 'descending'");
}

template <typename T> void bind_dot(py::module &m) {
  m.def(
      "dot", [](const T &x, const T &y) { return dot(x, y); }, py::arg("x"),
      py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_sort(py::module &m) {
  m.def(
      "sort",
      [](const T &x, const Variable &key, const std::string &order) {
        return sort(x, key, get_sort_order(order));
      },
      py::arg("x"), py::arg("key"), py::arg("order"),
      py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_sort_dim(py::module &m) {
  m.def(
      "sort",
      [](const T &x, const Dim &dim, const std::string &order) {
        return sort(x, dim, get_sort_order(order));
      },
      py::arg("x"), py::arg("key"), py::arg("order"),
      py::call_guard<py::gil_scoped_release>());
}

void bind_issorted(py::module &m) {
  m.def(
      "issorted",
      [](const Variable &x, const Dim dim, const std::string &order) {
        return issorted(x, dim, get_sort_order(order));
      },
      py::arg("x"), py::arg("dim"), py::arg("order") = "ascending",
      py::call_guard<py::gil_scoped_release>());
}

void bind_allsorted(py::module &m) {
  m.def(
      "allsorted",
      [](const Variable &x, const Dim dim, const std::string &order) {
        return allsorted(x, dim, get_sort_order(order));
      },
      py::arg("x"), py::arg("dim"), py::arg("order") = "ascending",
      py::call_guard<py::gil_scoped_release>());
}

void bind_midpoints(py::module &m) {
  m.def("midpoints", [](const Variable &x, const std::optional<Dim> dim) {
    return midpoints(x, dim);
  });
}

void init_operations(py::module &m) {
  bind_dot<Variable>(m);

  bind_sort<Variable>(m);
  bind_sort<DataArray>(m);
  bind_sort<Dataset>(m);
  bind_sort_dim<Variable>(m);
  bind_sort_dim<DataArray>(m);
  bind_sort_dim<Dataset>(m);
  bind_issorted(m);
  bind_allsorted(m);
  bind_midpoints(m);

  m.def(
      "get_slice_params",
      [](const Variable &var, const Variable &coord, const Variable &value) {
        const auto [dim, index] = get_slice_params(var.dims(), coord, value);
        return std::tuple{dim.name(), index};
      },
      py::call_guard<py::gil_scoped_release>());
  m.def("get_slice_params", [](const Variable &var, const Variable &coord,
                               const Variable &begin, const Variable &end) {
    const auto [dim, start, stop] =
        get_slice_params(var.dims(), coord, begin, end);
    // Do NOT release GIL since using py::slice
    return std::tuple{dim.name(), py::slice(start, stop, 1)};
  });

  m.def("where", &variable::where, py::arg("condition"), py::arg("x"),
        py::arg("y"), py::call_guard<py::gil_scoped_release>());
}
