// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "variable_view.h"
#include "dataset.h"
#include "except.h"
#include "pybind11.h"
#include "scipp/core/dtype.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

template <class T> struct mutable_span_methods {
  static void add(py::class_<scipp::span<T>> &span) {
    span.def("__setitem__", [](scipp::span<T> &self, const scipp::index i,
                               const T value) { self[i] = value; });
  }
};
template <class T> struct mutable_span_methods<const T> {
  static void add(py::class_<scipp::span<const T>> &) {}
};

template <class T> void declare_span(py::module &m, const std::string &suffix) {
  py::class_<scipp::span<T>> span(m, (std::string("span_") + suffix).c_str());
  span.def("__getitem__", &scipp::span<T>::operator[],
           py::return_value_policy::reference)
      .def("size", &scipp::span<T>::size)
      .def("__len__", &scipp::span<T>::size)
      .def("__iter__",
           [](const scipp::span<T> &self) {
             return py::make_iterator(self.begin(), self.end());
           })
      .def("__repr__",
           [](const scipp::span<T> &self) { return array_to_string(self); });
  mutable_span_methods<T>::add(span);
}

template <class T>
void declare_VariableView(py::module &m, const std::string &suffix) {
  py::class_<VariableView<T>> view(
      m, (std::string("VariableView_") + suffix).c_str());
  view.def("__repr__",
           [](const VariableView<T> &self) { return array_to_string(self); })
      .def("__getitem__", &VariableView<T>::operator[],
           py::return_value_policy::reference)
      .def("__setitem__", [](VariableView<T> &self, const scipp::index i,
                             const T value) { self[i] = value; })
      .def("__len__", &VariableView<T>::size)
      .def("__iter__", [](const VariableView<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      });
}

void init_variable_view(py::module &m) {
  declare_span<double>(m, "double");
  declare_span<float>(m, "float");
  declare_span<Bool>(m, "bool");
  declare_span<const double>(m, "double_const");
  declare_span<const long>(m, "long_const");
  declare_span<long>(m, "long");
  declare_span<const std::string>(m, "string_const");
  declare_span<std::string>(m, "string");
  declare_span<const Dim>(m, "Dim_const");
  declare_span<Dataset>(m, "Dataset");
  declare_span<Eigen::Vector3d>(m, "Eigen_Vector3d");

  declare_VariableView<double>(m, "double");
  declare_VariableView<float>(m, "float");
  declare_VariableView<int64_t>(m, "int64");
  declare_VariableView<int32_t>(m, "int32");
  declare_VariableView<std::string>(m, "string");
  declare_VariableView<Bool>(m, "bool");
  declare_VariableView<boost::container::small_vector<double, 8>>(
      m, "SmallVectorDouble8");
  declare_VariableView<Dataset>(m, "Dataset");
  declare_VariableView<Eigen::Vector3d>(m, "Eigen_Vector3d");
}
