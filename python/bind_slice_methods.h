// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "numpy.h"
#include "pybind11.h"
#include "scipp/core/dtype.h"
#include "scipp/core/slice.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

template <class T> auto dim_extent(const T &object, const Dim dim) {
  if constexpr (std::is_same_v<T, Dataset> || std::is_same_v<T, DatasetView>) {
    scipp::index extent = -1;
    if (object.dimensions().count(dim) > 0)
      extent = object.dimensions().at(dim);
    return extent;
  } else {
    return object.dims()[dim];
  }
}

template <class T>
auto from_py_slice(const T &source,
                   const std::tuple<Dim, const py::slice> &index) {
  const auto &[dim, indices] = index;
  size_t start, stop, step, slicelength;
  const auto size = dim_extent(source, dim);
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (step != 1)
    throw std::runtime_error("Step must be 1");
  return Slice(dim, start, stop);
}

template <class View> struct SetData {
  template <class T> struct Impl {
    static void apply(View &slice, const py::array &data) {
      if (slice.hasVariances())
        throw std::runtime_error("Data object contains variances, to set data "
                                 "values use the `values` property or provide "
                                 "a tuple of values and variances.");
      // Pybind11 converts py::array to py::array_t for us, with all sorts of
      // automatic conversions such as integer to double, if required.
      py::array_t<T> dataT(data);

      const auto &dims = slice.dims();
      const py::buffer_info info = dataT.request();
      const auto &shape = dims.shape();
      if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                      shape.end()))
        throw std::runtime_error(
            "Shape mismatch when setting data from numpy array.");

      auto buf = slice.template values<T>();
      copy_flattened<T>(dataT, buf);
    }
  };
};

// Helpers wrapped in struct to avoid unresolvable overloads.
template <class T> struct slicer {
  static auto get(T &self, const std::tuple<Dim, scipp::index> &index) {
    auto [dim, i] = index;
    auto sz = dim_extent(self, dim);
    if (i <= -sz || i >= sz) // index is out of range
      throw std::runtime_error("Dimension size is " +
                               std::to_string(dim_extent(self, dim)) +
                               ", can't treat " + std::to_string(i));
    if (i < 0)
      i = sz + i;
    return self.slice(Slice(dim, i));
  }

  static auto get_range(T &self,
                        const std::tuple<Dim, const py::slice> &index) {
    return self.slice(from_py_slice(self, index));
  }

  static void set_from_numpy(T &self,
                             const std::tuple<Dim, scipp::index> &index,
                             const py::array &data) {
    auto slice = slicer<T>::get(self, index);
    core::CallDType<double, float, int64_t, int32_t, bool>::apply<
        SetData<decltype(slice)>::template Impl>(slice.data().dtype(), slice,
                                                 data);
  }

  static void
  set_range_from_numpy(T &self, const std::tuple<Dim, const py::slice> &index,
                       const py::array &data) {
    auto slice = slicer<T>::get_range(self, index);
    core::CallDType<double, float, int64_t, int32_t, bool>::apply<
        SetData<decltype(slice)>::template Impl>(slice.data().dtype(), slice,
                                                 data);
  }

  template <class Other>
  static void set(T &self, const std::tuple<Dim, scipp::index> &index,
                  const Other &data) {
    auto slice = slicer<T>::get(self, index);
    slice.assign(data);
  }

  template <class Other>
  static void set_range(T &self, const std::tuple<Dim, const py::slice> &index,
                        const Other &data) {
    auto slice = slicer<T>::get_range(self, index);
    slice.assign(data);
  }
};

template <class T, class... Ignored>
void bind_slice_methods(pybind11::class_<T, Ignored...> &c) {
  c.def("__getitem__", &slicer<T>::get, py::keep_alive<0, 1>());
  c.def("__getitem__", &slicer<T>::get_range, py::keep_alive<0, 1>());
  if constexpr (!std::is_same_v<T, Dataset> &&
                !std::is_same_v<T, DatasetView>) {
    c.def("__setitem__", &slicer<T>::set_from_numpy);
    c.def("__setitem__", &slicer<T>::set_range_from_numpy);
    c.def("__setitem__", &slicer<T>::template set<Variable>);
    c.def("__setitem__", &slicer<T>::template set<VariableView>);
    c.def("__setitem__", &slicer<T>::template set_range<Variable>);
    c.def("__setitem__", &slicer<T>::template set_range<VariableView>);
  }
  if constexpr (std::is_same_v<T, DataArray> ||
                std::is_same_v<T, DataArrayView>) {
    c.def("__setitem__", &slicer<T>::template set<DataArrayView>);
    c.def("__setitem__", &slicer<T>::template set_range<DataArrayView>);
  }
  if constexpr (std::is_same_v<T, Dataset> || std::is_same_v<T, DatasetView>) {
    c.def("__setitem__", &slicer<T>::template set<DatasetView>);
    c.def("__setitem__", &slicer<T>::template set_range<DatasetView>);
  }
}
