// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element_array_view.h"
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/dataset/dataset.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

template <class T> struct mutable_span_methods {
  static void add(py::class_<scipp::span<T>> &span) {
    span.def("__setitem__", [](scipp::span<T> &self, const scipp::index i,
                               const T value) { self[i] = value; });
    // This is to support, e.g., `var.values[i] = np.zeros(4)`.
    if constexpr (is_events_v<T>)
      span.def("__setitem__",
               [](scipp::span<T> &self, const scipp::index i,
                  const std::vector<typename T::value_type> &value) {
                 self[i].assign(value.begin(), value.end());
               });
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
void declare_ElementArrayView(py::module &m, const std::string &suffix) {
  py::class_<ElementArrayView<T>> view(
      m, (std::string("ElementArrayView_") + suffix).c_str());
  view.def(
          "__repr__",
          [](const ElementArrayView<T> &self) { return array_to_string(self); })
      .def("__getitem__", &ElementArrayView<T>::operator[],
           py::return_value_policy::reference)
      .def("__setitem__", [](ElementArrayView<T> &self, const scipp::index i,
                             const T value) { self[i] = value; })
      .def("__len__", &ElementArrayView<T>::size)
      .def("__iter__", [](const ElementArrayView<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      });
  if constexpr (is_events_v<T>)
    view.def("__setitem__",
             [](const ElementArrayView<T> &self, const scipp::index i,
                const std::vector<typename T::value_type> &value) {
               self[i].assign(value.begin(), value.end());
             });
}

void init_element_array_view(py::module &m) {
  declare_span<double>(m, "double");
  declare_span<float>(m, "float");
  declare_span<bool>(m, "bool");
  declare_span<const double>(m, "double_const");
  declare_span<const long>(m, "long_const");
  declare_span<long>(m, "long");
  declare_span<const std::string>(m, "string_const");
  declare_span<std::string>(m, "string");
  declare_span<const Dim>(m, "Dim_const");
  declare_span<DataArray>(m, "DataArray");
  declare_span<Dataset>(m, "Dataset");
  declare_span<Eigen::Vector3d>(m, "Eigen_Vector3d");
  declare_span<Eigen::Matrix3d>(m, "Eigen_Matrix3d");
  declare_span<event_list<double>>(m, "event_double");
  declare_span<event_list<float>>(m, "event_float");
  declare_span<event_list<int64_t>>(m, "event_int64_t");

  declare_ElementArrayView<double>(m, "double");
  declare_ElementArrayView<float>(m, "float");
  declare_ElementArrayView<int64_t>(m, "int64");
  declare_ElementArrayView<int32_t>(m, "int32");
  declare_ElementArrayView<std::string>(m, "string");
  declare_ElementArrayView<bool>(m, "bool");
  declare_ElementArrayView<event_list<double>>(m, "event_double");
  declare_ElementArrayView<event_list<float>>(m, "event_float");
  declare_ElementArrayView<event_list<int64_t>>(m, "event_int64_t");
  declare_ElementArrayView<DataArray>(m, "DataArray");
  declare_ElementArrayView<Dataset>(m, "Dataset");
  declare_ElementArrayView<Eigen::Vector3d>(m, "Eigen_Vector3d");
  declare_ElementArrayView<Eigen::Matrix3d>(m, "Eigen_Matrix3d");
}
