// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cstddef>
#include <functional>

#include "scipp/core/parallel.h"
#include "scipp/variable/variable.h"

#include "pybind11.h"

namespace py = pybind11;

namespace scipp::detail {
/// Copy the bytes of src into dst.
template <class Source, class Destination>
void reinterpret_copy_element(const Source &src, Destination &&dst) noexcept {
  // Go through std::byte in order to avoid UB of casting Destination
  // to Source or vice versa. Should be optimized away.
  static_assert(sizeof(Destination) == sizeof(Source));
  auto src_ptr = reinterpret_cast<const std::byte *>(&src);
  std::remove_reference_t<Destination> &dst_ = dst;
  auto dst_ptr = reinterpret_cast<std::byte *>(&dst_);
  std::copy(src_ptr, src_ptr + sizeof(Source), dst_ptr);
}

template <bool reinterpret, class Source, class Destination>
void copy_element(const Source &src, Destination &&dst) noexcept(reinterpret) {
  if constexpr (reinterpret) {
    reinterpret_copy_element(src, std::forward<Destination>(dst));
  } else {
    std::forward<Destination>(dst) = src;
  }
}
} // namespace scipp::detail

using namespace scipp;

/// Map C++ types to Python types to perform conversion / reinterpret casting
/// between scipp containers and numpy arrays.
template <class T> struct ElementTypeMap {
  using CppType = T;
  using PyType = T;
  constexpr static bool reinterpret = false;

  static void check_assignable(const py::object &, const units::Unit &) {}
};

template <> struct ElementTypeMap<scipp::core::time_point> {
  using CppType = scipp::core::time_point;
  using PyType = int64_t;
  constexpr static bool reinterpret = true;

  static void check_assignable(const py::object &obj, units::Unit unit);
};

template <bool reinterpret, class T, class View>
void copy_flattened_0d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  auto it = view.begin();
  detail::copy_element<reinterpret>(r(), *it);
}

template <bool reinterpret, class T, class View>
void copy_flattened_1d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  auto it = view.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i, ++it) {
    detail::copy_element<reinterpret>(r(i), *it);
  }
}

template <bool reinterpret, class T, class View>
void copy_flattened_2d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  const auto begin = view.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, r.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * r.shape(1);
        for (ssize_t i = range.begin(); i < range.end(); ++i)
          for (ssize_t j = 0; j < r.shape(1); ++j, ++it)
            detail::copy_element<reinterpret>(r(i, j), *it);
      });
}

template <bool reinterpret, class T, class View>
void copy_flattened_3d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  auto it = view.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i)
    for (ssize_t j = 0; j < r.shape(1); ++j)
      for (ssize_t k = 0; k < r.shape(2); ++k, ++it)
        detail::copy_element<reinterpret>(r(i, j, k), *it);
}

template <bool reinterpret, class T, class View>
void copy_flattened_4d(const py::array_t<T> &data, View &&view) {
  auto r = data.unchecked();
  auto it = view.begin();
  for (ssize_t i = 0; i < r.shape(0); ++i)
    for (ssize_t j = 0; j < r.shape(1); ++j)
      for (ssize_t k = 0; k < r.shape(2); ++k)
        for (ssize_t l = 0; l < r.shape(3); ++l, ++it)
          detail::copy_element<reinterpret>(r(i, j, k, l), *it);
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
  const auto [data_begin, data_end] = memory_begin_end<std::byte>(buffer_info);
  const auto begin = view.begin();
  const auto end = view.end();
  if (begin == end) {
    return false;
  }
  const auto view_begin = reinterpret_cast<const std::byte *>(&*begin);
  const auto view_end = reinterpret_cast<const std::byte *>(&*end);
  // Note the use of std::less, pointer comparison with operator< may be
  // undefined behavior with pointers from different arrays.
  return std::less<>()(data_begin, view_end) &&
         std::greater_equal<>()(data_end, view_begin);
}

/// Copy all elements from src into dst.
/// Performs proper conversions from element type of src to element type of dst
/// if reinterpret=false.
/// Otherwise, elements in src are copied bitwise into dst.
template <bool reinterpret, class T, class View>
void copy_flattened(const py::array_t<T> &src, View &&dst) {
  if (scipp::size(dst) != src.size())
    throw std::runtime_error(
        "Numpy data size does not match size of target object.");

  const auto dispatch = [](const py::array_t<T> &src_, View &&dst_) {
    switch (src_.ndim()) {
    case 0:
      return copy_flattened_0d<reinterpret>(src_, std::forward<View>(dst_));
    case 1:
      return copy_flattened_1d<reinterpret>(src_, std::forward<View>(dst_));
    case 2:
      return copy_flattened_2d<reinterpret>(src_, std::forward<View>(dst_));
    case 3:
      return copy_flattened_3d<reinterpret>(src_, std::forward<View>(dst_));
    case 4:
      return copy_flattened_4d<reinterpret>(src_, std::forward<View>(dst_));
    default:
      throw std::runtime_error("Numpy array has more dimensions than supported "
                               "in the current implementation.");
    }
  };
  dispatch(memory_overlaps(src, dst) ? py::array_t<T>(src.request()) : src,
           std::forward<View>(dst));
}
