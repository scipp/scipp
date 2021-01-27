// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <functional>

#include "scipp/core/parallel.h"
#include "scipp/variable/variable.h"

#include "pybind11.h"

namespace py = pybind11;
using namespace scipp;

template <class T, class View>
void copy_flattened_0d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  auto it = view.begin();
  *it = r();
}

template <class T, class View>
void copy_flattened_1d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  auto it = view.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i, ++it)
    *it = r(i);
}

template <class T, class View>
void copy_flattened_2d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  const auto begin = view.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, r.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * r.shape(1);
        for (ssize_t i = range.begin(); i < range.end(); ++i)
          for (ssize_t j = 0; j < r.shape(1); ++j, ++it)
            *it = r(i, j);
      });
}

template <class T, class View>
void copy_flattened_3d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  auto it = view.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i)
    for (ssize_t j = 0; j < r.shape(1); ++j)
      for (ssize_t k = 0; k < r.shape(2); ++k, ++it)
        *it = r(i, j, k);
}

template <class T, class View>
void copy_flattened_4d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  auto it = view.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i)
    for (ssize_t j = 0; j < r.shape(1); ++j)
      for (ssize_t k = 0; k < r.shape(2); ++k)
        for (ssize_t l = 0; l < r.shape(3); ++l, ++it)
          *it = r(i, j, k, l);
}

template <class T> auto memory_begin_end(const py::buffer_info &info) {
  auto *begin = static_cast<const T *>(info.ptr);
  auto *end = static_cast<const T *>(info.ptr);
  const auto &shape = info.shape;
  const auto &strides = info.strides;
  for (scipp::index i = 0; i < scipp::size(shape); ++i)
    if (strides[i] < 0)
      begin += shape[i] * strides[i];
    else
      end += shape[i] * strides[i];
  return std::pair{begin, end};
}

template <class T, class View>
bool memory_overlaps(const py::array_t<T> &data, const View &view) {
  const auto &buffer_info = data.request();
  const auto [data_begin, data_end] = memory_begin_end<T>(buffer_info);
  const auto begin = view.begin();
  const auto end = view.end();
  if (begin == end) {
    return false;
  }
  const auto view_begin = &*begin;
  const auto view_end = &*end;
  // Note the use of std::less, pointer comparison with operator< may be
  // undefined behavior with pointers from different arrays.
  return std::less<const T *>()(data_begin, view_end) &&
         std::greater_equal<const T *>()(data_end, view_begin);
}

template <class T, class View>
void copy_flattened(py::array_t<T> data, View &&view) {
  if (scipp::size(view) != data.size())
    throw std::runtime_error(
        "Numpy data size does not match size of target object.");
  if (memory_overlaps(data, view))
    data = py::array_t<T>(data.request());
  switch (data.ndim()) {
  case 0:
    return copy_flattened_0d(data, view);
  case 1:
    return copy_flattened_1d(data, view);
  case 2:
    return copy_flattened_2d(data, view);
  case 3:
    return copy_flattened_3d(data, view);
  case 4:
    return copy_flattened_4d(data, view);
  default:
    throw std::runtime_error("Numpy array has more dimensions than supported "
                             "in the current implementation.");
  }
}
