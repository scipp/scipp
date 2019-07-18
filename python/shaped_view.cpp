// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "shaped_view.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::python;

namespace py = pybind11;

template <class T>
void bind_ShapedView(py::module &m, const std::string &suffix) {
  py::class_<BufferView<T>> view(
      m, (std::string("shaped_view_") + suffix).c_str());
  span.def("__getitem__", &scipp::span<T>::operator[],
           py::return_value_policy::reference)
      .def("__len__", &scipp::span<T>::size)
      .def("__iter__",
           [](const scipp::span<T> &self) {
             return py::make_iterator(self.begin(), self.end());
           })
      .def_property_readonly(
          "shape", [](const Dimensions &self) { return self.shape(); },
          "Return the shape of the space defined by self. If there is a sparse "
          "dimension the shape of the dense subspace is returned.")
      .def("__repr__",
           [](const scipp::span<T> &self) { return array_to_string(self); });
  mutable_span_methods<T>::add(span);
}

void init_shaped_view(py::module &m) {
  py::class_<BufferView>(m, "ShapedView")
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
             std::string out = "Dimensions = " + to_string(self);
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
      .def(py::self != py::self);
}
