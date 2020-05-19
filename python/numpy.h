// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

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

template <class T, class View>
void copy_flattened(const py::array_t<T> &data, View &&view) {
  if (scipp::size(view) != data.size())
    throw std::runtime_error(
        "Numpy data size does not match size of target object.");
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
