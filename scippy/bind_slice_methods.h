// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPPY_BIND_SLICE_METHODS_H
#define SCIPPY_BIND_SLICE_METHODS_H

#include <pybind11/pybind11.h>

#include "numpy.h"
#include "scipp/core/dtype.h"
#include "tag_util.h"
#include "variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::core;

template <class T>
auto from_py_slice(const T &source,
                   const std::tuple<Dim, const py::slice> &index) {
  const auto & [ dim, indices ] = index;
  size_t start, stop, step, slicelength;
  const auto size = source.dims()[dim];
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (step != 1)
    throw std::runtime_error("Step must be 1");
  return Slice(dim, start, stop);
}

template <class Proxy> struct SetData {
  template <class T> struct Impl {
    static void apply(Proxy &slice, const py::array &data) {
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
template <class Proxy> auto set_data(Proxy &&) { return SetData<Proxy>{}; }

// Helpers wrapped in struct to avoid unresolvable overloads.
template <class T> struct slicer {
  static auto get(T &self, const std::tuple<Dim, scipp::index> &index) {
    auto[dim, i] = index;
    auto sz = self.dims()[dim];
    if (i <= -sz || i >= sz) // index is out of range
      throw std::runtime_error("Dimension size is " +
                               std::to_string(self.dims()[dim]) +
                               ", can't treat " + std::to_string(i));
    if (i < 0)
      i = sz + i;
    return self.slice(Slice(dim, i));
  }
  static auto get_range(T &self,
                        const std::tuple<Dim, const py::slice> &index) {
    return self.slice(from_py_slice(self, index));
  }

  static void set(T &self, const std::tuple<Dim, scipp::index> &index,
                  const py::array &data) {
    auto slice = slicer<T>::get(self, index);
    CallDType<double, float, int64_t, int32_t, bool>::apply<
        SetData<decltype(slice)>::template Impl>(slice.data().dtype(), slice,
                                                 data);
  }

  static void set_range(T &self, const std::tuple<Dim, const py::slice> &index,
                        const py::array &data) {
    auto slice = slicer<T>::get_range(self, index);
    CallDType<double, float, int64_t, int32_t, bool>::apply<
        SetData<decltype(slice)>::template Impl>(slice.data().dtype(), slice,
                                                 data);
  }
};

template <class T, class... Ignored>
void bind_slice_methods(pybind11::class_<T, Ignored...> &c) {
  c.def("__getitem__", &slicer<T>::get);
  c.def("__getitem__", &slicer<T>::get_range);
  c.def("__setitem__", &slicer<T>::set);
  c.def("__setitem__", &slicer<T>::set_range);
}

#endif // SCIPPY_BIND_SLICE_METHODS_H
