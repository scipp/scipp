// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#include "scipp/variable/shape.h"
#include "docstring.h"
#include "pybind11.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/variable.h"

#include "dim.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

namespace {

template <class T> void bind_broadcast(py::module &m) {
  m.def(
      "broadcast",
      [](const T &self, const std::vector<std::string> &labels,
         const std::vector<scipp::index> &shape) {
        return broadcast(self, make_dims(labels, shape));
      },
      py::arg("x"), py::arg("dims"), py::arg("shape"));
}

template <class T> void bind_concat(py::module &m) {
  m.def(
      "concat",
      [](const std::vector<T> &x, const std::string &dim) {
        return concat(x, Dim{dim});
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_fold(pybind11::module &mod) {
  mod.def(
      "fold",
      [](const T &self, const std::string &dim,
         const std::vector<std::string> &labels,
         const std::vector<scipp::index> &shape) {
        return fold(self, Dim{dim}, make_dims(labels, shape));
      },
      py::arg("x"), py::arg("dim"), py::arg("dims"), py::arg("shape"),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_flatten(pybind11::module &mod) {
  mod.def(
      "flatten",
      [](const T &self, const std::optional<std::vector<std::string>> &dims,
         const std::string &to) {
        if (dims.has_value())
          return flatten(self, to_dim_type(*dims), Dim{to});
        // If no dims are given then we flatten all dims. For variables we just
        // provide a list of all labels. DataArrays are different, as the
        // behavior in the degenerate case of a 0-D 'self' must distinguish
        // between flattening "zero dims" and "all dims". The latter is
        // specified using std::nullopt.
        if constexpr (std::is_same_v<T, Variable>)
          return flatten(self, self.dims().labels(), Dim{to});
        else
          return flatten(self, std::nullopt, Dim{to});
      },
      py::arg("x"), py::arg("dims"), py::arg("to"),
      py::call_guard<py::gil_scoped_release>());
}

template <class T> void bind_transpose(pybind11::module &mod) {
  mod.def(
      "transpose",
      [](const T &self, const std::vector<std::string> &dims) {
        return transpose(self, to_dim_type(dims));
      },
      py::arg("x"), py::arg("dims") = std::vector<std::string>{});
}

template <class T> void bind_squeeze(pybind11::module &mod) {
  mod.def(
      "squeeze",
      [](const T &self, const std::optional<std::vector<std::string>> &dims) {
        return squeeze(self, dims.has_value()
                                 ? std::optional{to_dim_type(*dims)}
                                 : std::optional<std::vector<Dim>>{});
      },
      py::arg("x"), py::arg("dims") = std::nullopt);
}
} // namespace

void init_shape(py::module &m) {
  bind_broadcast<Variable>(m);
  bind_concat<Variable>(m);
  bind_concat<DataArray>(m);
  bind_concat<Dataset>(m);
  bind_fold<Variable>(m);
  bind_fold<DataArray>(m);
  bind_flatten<Variable>(m);
  bind_flatten<DataArray>(m);
  bind_transpose<Variable>(m);
  bind_transpose<DataArray>(m);
  bind_transpose<Dataset>(m);
  bind_squeeze<Variable>(m);
  bind_squeeze<DataArray>(m);
  bind_squeeze<Dataset>(m);
}
