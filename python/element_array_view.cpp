// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element_array_view.h"
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

namespace {
template <class T> struct is_bins : std::false_type {};
template <class T> struct is_bins<core::bin<T>> : std::true_type {};
} // namespace

template <class T>
void declare_ElementArrayView(py::module &m, const std::string &suffix) {
  py::class_<ElementArrayView<T>> view(
      m, (std::string("ElementArrayView_") + suffix).c_str());
  view.def(
          "__repr__",
          [](const ElementArrayView<T> &self) { return array_to_string(self); })
      .def("__getitem__", &ElementArrayView<T>::operator[],
           py::return_value_policy::reference)
      .def("__len__", &ElementArrayView<T>::size)
      .def("__iter__", [](const ElementArrayView<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      });
  view.def("__setitem__",
           [](ElementArrayView<T> &self, const scipp::index i, const T value) {
             if constexpr (is_bins<T>::value || std::is_const_v<T>)
               throw std::invalid_argument(
                   "assignment destination is read-only");
             else
               self[i] = value;
           });
}

void init_element_array_view(py::module &m) {
  declare_ElementArrayView<double>(m, "double");
  declare_ElementArrayView<float>(m, "float");
  declare_ElementArrayView<int64_t>(m, "int64");
  declare_ElementArrayView<int32_t>(m, "int32");
  declare_ElementArrayView<std::string>(m, "string");
  declare_ElementArrayView<bool>(m, "bool");
  declare_ElementArrayView<Variable>(m, "Variable");
  declare_ElementArrayView<DataArray>(m, "DataArray");
  declare_ElementArrayView<Dataset>(m, "Dataset");
  declare_ElementArrayView<Eigen::Vector3d>(m, "Eigen_Vector3d");
  declare_ElementArrayView<Eigen::Matrix3d>(m, "Eigen_Matrix3d");
  declare_ElementArrayView<bucket<Variable>>(m, "bin_Variable");
  declare_ElementArrayView<bucket<DataArray>>(m, "bin_DataArray");
  declare_ElementArrayView<bucket<Dataset>>(m, "bin_Dataset");

  declare_ElementArrayView<const double>(m, "double_const");
  declare_ElementArrayView<const float>(m, "float_const");
  declare_ElementArrayView<const int64_t>(m, "int64_const");
  declare_ElementArrayView<const int32_t>(m, "int32_const");
  declare_ElementArrayView<const std::string>(m, "string_const");
  declare_ElementArrayView<const bool>(m, "bool_const");
  declare_ElementArrayView<const Variable>(m, "Variable_const");
  declare_ElementArrayView<const DataArray>(m, "DataArray_const");
  declare_ElementArrayView<const Dataset>(m, "Dataset_const");
  declare_ElementArrayView<const Eigen::Vector3d>(m, "Eigen_Vector3d_const");
  declare_ElementArrayView<const Eigen::Matrix3d>(m, "Eigen_Matrix3d_const");
  declare_ElementArrayView<const bucket<Variable>>(m, "bin_Variable_const");
  declare_ElementArrayView<const bucket<DataArray>>(m, "bin_DataArray_const");
  declare_ElementArrayView<const bucket<Dataset>>(m, "bin_Dataset_const");
}
