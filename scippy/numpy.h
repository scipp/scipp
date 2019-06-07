// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPPY_NUMPY_H
#define SCIPPY_NUMPY_H

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::core;

template <class T, class Proxy>
void copy_flattened_0d(const py::array_t<T> &data, Proxy &&proxy) {
  auto r = data.unchecked();
  auto it = proxy.begin();
  *it = r();
}

template <class T, class Proxy>
void copy_flattened_1d(const py::array_t<T> &data, Proxy &&proxy) {
  auto r = data.unchecked();
  auto it = proxy.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i, ++it)
    *it = r(i);
}

template <class T, class Proxy>
void copy_flattened_2d(const py::array_t<T> &data, Proxy &&proxy) {
  auto r = data.unchecked();
  auto it = proxy.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i)
    for (ssize_t j = 0; j < r.shape(1); ++j, ++it)
      *it = r(i, j);
}

template <class T, class Proxy>
void copy_flattened_3d(const py::array_t<T> &data, Proxy &&proxy) {
  auto r = data.unchecked();
  auto it = proxy.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i)
    for (ssize_t j = 0; j < r.shape(1); ++j)
      for (ssize_t k = 0; k < r.shape(2); ++k, ++it)
        *it = r(i, j, k);
}

template <class T, class Proxy>
void copy_flattened(const py::array_t<T> &data, Proxy &&proxy) {
  if (proxy.size() != data.size())
    throw std::runtime_error(
        "Numpy data size does not match size of target object.");
  switch (data.ndim()) {
  case 0:
    return copy_flattened_0d(data, proxy);
  case 1:
    return copy_flattened_1d(data, proxy);
  case 2:
    return copy_flattened_2d(data, proxy);
  case 3:
    return copy_flattened_3d(data, proxy);
  default:
    throw std::runtime_error("Numpy array has more dimensions than supported "
                             "in the current implementation.");
  }
}

#endif // SCIPPY_NUMPY_H
