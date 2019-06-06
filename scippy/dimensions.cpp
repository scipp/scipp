// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "dimensions.h"
#include "except.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

void init_dimensions(py::module &m) {
  py::class_<Dimensions>(m, "Dimensions")
      .def(py::init<>())
      .def(py::init([](const std::vector<Dim> &labels,
                       const std::vector<scipp::index> &shape) {
             return Dimensions(labels, shape);
           }),
           py::arg("labels"), py::arg("shape"))
      .def("__repr__",
           [](const Dimensions &self) {
             std::string out = "Dimensions = " + to_string(self, ".");
             return out;
           })
      .def("__contains__", [](const Dimensions &self,
                              const Dim dim) { return self.contains(dim); })
      .def("__getitem__",
           py::overload_cast<const Dim>(&Dimensions::operator[], py::const_))
      .def_property_readonly(
          "labels", [](const Dimensions &self) { return self.labels(); },
          "The read-only tags labelling"
          "the different dimensions of the underlying "
          "Variable or VariableProxy, e.g. [Dim.Y, Dim.X].")
      .def_property_readonly(
          "shape", [](const Dimensions &self) { return self.shape(); },
          "The read-only sizes of each dimension of the "
          "underlying Variable or VariableProxy.")
      .def("add", &Dimensions::add,
           "Add a new dimension, which will be the outermost dimension.")
      .def(py::self == py::self)
      .def(py::self != py::self);
}
