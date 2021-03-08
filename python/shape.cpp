// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#include "scipp/variable/shape.h"
#include "docstring.h"
#include "pybind11.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

namespace {

Dimensions dict_to_dims(const py::dict &map) {
  Dimensions dims;
  for (const auto &item : map)
    dims.addInner(item.first.cast<Dim>(), item.second.cast<scipp::index>());
  return dims;
}

template <class T> void bind_broadcast(py::module &m) {
  m.def(
      "broadcast",
      [](const typename T::const_view_type &self,
         const std::vector<Dim> &labels,
         const std::vector<scipp::index> &shape) {
        Dimensions dims(labels, shape);
        return broadcast(self, dims);
      },
      py::arg("x"), py::arg("dims"), py::arg("shape"));
}

template <class T> void bind_concatenate(py::module &m) {
  m.def(
      "concatenate",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y,
         const Dim dim) { return concatenate(x, y, dim); },
      py::arg("x"), py::arg("y"), py::arg("dim"),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_reshape(pybind11::module &mod) {
  mod.def(
      "reshape",
      [](const T &self, const py::dict &sizes) {
        Dimensions new_dims;
        for (const auto &item : sizes)
          new_dims.addInner(item.first.cast<Dim>(),
                            item.second.cast<scipp::index>());
        py::gil_scoped_release release; // release only *after* using py::cast
        return reshape(self, new_dims);
      },
      py::arg("x"), py::arg("sizes"));
  mod.def(
      "split",
      [](const T &self, const Dim dim, const py::dict &sizes) {
        const auto new_dims = dict_to_dims(sizes);
        py::gil_scoped_release release; // release only *after* using py::cast
        return split(self, dim, new_dims);
      },
      py::arg("x"), py::arg("dim"), py::arg("sizes"));
  mod.def(
      "flatten",
      [](const T &self, const std::vector<Dim> &dims, const Dim &to) {
        return flatten(self, dims, to);
      },
      py::arg("x"), py::arg("dims"), py::arg("to"),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_split(pybind11::module &mod) {
  mod.def(
      "split",
      [](const T &self, const Dim dim, const py::dict &sizes) {
        const auto new_dims = dict_to_dims(sizes);
        py::gil_scoped_release release; // release only *after* using py::cast
        return split(self, dim, new_dims);
      },
      py::arg("x"), py::arg("dim"), py::arg("sizes"));
}

template <class T> void bind_flatten(pybind11::module &mod) {
  mod.def(
      "flatten",
      [](const T &self, const std::vector<Dim> &dims, const Dim &to) {
        return flatten(self, dims, to);
      },
      py::arg("x"), py::arg("dims"), py::arg("to"),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_transpose(pybind11::module &mod) {
  mod.def(
      "transpose",
      [](const T &self, const std::vector<Dim> &dims) {
        return transpose(self, dims);
      },
      py::arg("x"), py::arg("dims") = std::vector<Dim>{});
}
} // namespace

void init_shape(py::module &m) {
  bind_broadcast<Variable>(m);
  bind_concatenate<Variable>(m);
  bind_concatenate<DataArray>(m);
  bind_concatenate<Dataset>(m);
  bind_reshape<Variable>(m);
  bind_reshape<VariableView>(m);
  bind_split<DataArray>(m);
  bind_split<DataArrayView>(m);
  bind_flatten<DataArray>(m);
  bind_flatten<DataArrayView>(m);
  bind_transpose<Variable>(m);
  bind_transpose<VariableView>(m);
}
