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
      .def_property_readonly_static(
          "Sparse", [](py::object /* self */) { return Dimensions::Sparse; },
          "Dummy label to use when specifying the shape for sparse data.")
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
      .def("__contains__",
           [](const Dimensions &self, const Dim dim) {
             return self.contains(dim);
           },
           "Return true if `dim` is one of the labels in *this.")
      .def("__getitem__",
           py::overload_cast<const Dim>(&Dimensions::operator[], py::const_))
      .def_property_readonly("sparse", &Dimensions::sparse,
                             "Return True if there is a sparse dimension.")
      .def_property_readonly("sparseDim", &Dimensions::sparseDim,
                             "Return the label of a potential sparse "
                             "dimension, Dim.Invalid otherwise.")
      .def_property_readonly(
          "shape", [](const Dimensions &self) { return self.shape(); },
          "Return the shape of the space defined by self. If there is a sparse "
          "dimension the shape of the dense subspace is returned.")
      .def_property_readonly(
          "labels", [](const Dimensions &self) { return self.labels(); },
          "Return the labels of the space defined by self, including the label "
          "of a potential sparse dimension.")
      .def_property_readonly(
          "denseLabels",
          [](const Dimensions &self) { return self.denseLabels(); },
          "Return the labels of the space defined by self, excluding the label "
          "of a potential sparse dimension")
      .def("add", &Dimensions::add,
           "Add a new dimension, which will be the outermost dimension.")
      .def(py::self == py::self)
      .def(py::self != py::self);
}
