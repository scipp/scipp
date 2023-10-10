// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cstddef>
#include <functional>

#include "scipp/common/index_composition.h"
#include "scipp/core/parallel.h"
#include "scipp/variable/variable.h"

#include "py_object.h"
#include "pybind11.h"

namespace py = pybind11;

using namespace scipp;

/// Map C++ types to Python types to perform conversion between scipp containers
/// and numpy arrays.
template <class T> struct ElementTypeMap {
  using PyType = T;
  constexpr static bool convert = false;

  static void check_assignable(const py::object &, const units::Unit &) {}
};

template <> struct ElementTypeMap<scipp::core::time_point> {
  using PyType = int64_t;
  constexpr static bool convert = true;

  static void check_assignable(const py::object &obj, units::Unit unit);
};

template <> struct ElementTypeMap<scipp::python::PyObject> {
  using PyType = py::object;
  constexpr static bool convert = true;

  static void check_assignable(const py::object &, const units::Unit &) {}
};

template <bool convert, class Source, class Destination>
void copy_element(const Source &src, Destination &&dst) {
  using std::to_string;
  if constexpr (convert) {
    dst = std::remove_reference_t<Destination>{src};
  } else {
    std::forward<Destination>(dst) = src;
  }
}

/// Cast a py::object referring to an array to py::array_t<auto> if supported.
/// Otherwise, copies the contents into a std::vector<auto>.
template <class T>
auto cast_to_array_like(const py::object &obj, const units::Unit unit) {
  using TM = ElementTypeMap<T>;
  using PyType = typename TM::PyType;
  TM::check_assignable(obj, unit);
  if constexpr (std::is_same_v<T, core::time_point>) {
    // pbj.cast<py::array_t<PyType> does not always work because
    // numpy.datetime64.__int__ delegates to datetime.datetime if the unit is
    // larger than ns and that cannot be converted to long.
    return obj.cast<py::array>()
        .attr("astype")(py::dtype::of<PyType>())
        .template cast<py::array_t<PyType>>();
  } else if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
    // Casting to py::array_t applies all sorts of automatic conversions
    // such as integer to double, if required.
    return obj.cast<py::array_t<PyType>>();
  } else {
    // py::array only supports POD types. Use a simple but expensive
    // solution for other types.
    // TODO Related to #290, we should properly support
    //  multi-dimensional input, and ignore bad shapes.
    try {
      return obj.cast<const std::vector<PyType>>();
    } catch (std::runtime_error &) {
      const auto &array = obj.cast<py::array>();
      std::ostringstream oss;
      oss << "Unable to assign object of dtype " << py::str(array.dtype())
          << " to " << scipp::core::dtype<T>;
      throw std::invalid_argument(oss.str());
    }
  }
}

template <class T> struct typed_buffer {
  const T *ptr;
  ssize_t ndim;
  scipp::span<ssize_t> shape;

  typed_buffer(const T *ptr_, const ssize_t ndim_, scipp::span<ssize_t> shape_,
               py::buffer_info base_buffer_)
      : ptr{ptr_}, ndim{ndim_}, shape{shape_}, m_base_buffer{
                                                   std::move(base_buffer_)} {}

  [[nodiscard]] ssize_t stride(const ssize_t dim) const noexcept {
    return m_base_buffer.strides[dim] / m_base_buffer.itemsize;
  }

private:
  py::buffer_info m_base_buffer;
};

template <class T> auto request_typed_buffer(const py::array_t<T> &array) {
  // Order of operations such that we can safely move base_buffer.
  auto base_buffer = array.request();
  auto ptr = reinterpret_cast<const T *>(base_buffer.ptr);
  auto ndim = base_buffer.ndim;
  scipp::span<ssize_t> shape = base_buffer.shape;
  return typed_buffer(ptr, ndim, shape, std::move(base_buffer));
}

template <bool convert, class T, class Out>
void copy_flattened_0d(const typed_buffer<T> &src, Out &out) {
  copy_element<convert>(*src.ptr, *out);
}

template <bool convert, class T, class Out>
void copy_flattened_inner_dim(const typed_buffer<T> &src, Out &out,
                              ssize_t length, const ssize_t dim,
                              const ssize_t offset) {
  const auto stride = src.stride(dim);
  length = length == -1 ? src.shape[dim] : length;
  for (scipp::index i = 0; i < length; ++i) {
    copy_element<convert>(src.ptr[i * stride + offset], *out);
    ++out;
  }
}

template <bool convert, class T, class Out>
void copy_flattened_middle_dims(const typed_buffer<T> &src, Out &out,
                                ssize_t length, const ssize_t dim,
                                const ssize_t offset) {
  if (dim + 1 == src.ndim)
    copy_flattened_inner_dim<convert>(src, out, length, dim, offset);
  else {
    const auto stride = src.stride(dim);
    length = length == -1 ? src.shape[dim] : length;
    for (scipp::index i = 0; i < length; ++i)
      copy_flattened_middle_dims<convert>(src, out, -1, dim + 1,
                                          i * stride + offset);
  }
}

template <bool convert, class T, class Out>
void copy_flattened(const typed_buffer<T> &src, Out &out) {
  if (src.ndim == 0)
    copy_flattened_0d<convert>(src, out);
  else {
    const auto stride = src.stride(0);
    core::parallel::parallel_for(
        core::parallel::blocked_range(0, src.shape[0]), [&](const auto &range) {
          auto block_out = out + range.begin() * stride;
          copy_flattened_middle_dims<convert>(src, block_out,
                                              range.end() - range.begin(), 0,
                                              range.begin() * stride);
        });
  }
}

template <class T> auto memory_begin_end(const py::buffer_info &info) {
  auto *begin = static_cast<const T *>(info.ptr);
  auto *end = static_cast<const T *>(info.ptr);
  const auto [begin_offset, end_offset] =
      memory_bounds(info.shape.begin(), info.shape.end(), info.strides.begin());
  return std::pair{begin + begin_offset, end + end_offset};
}

template <class T, class View>
bool memory_overlaps(const py::array_t<T> &data, const View &view) {
  const auto &buffer_info = data.request();
  const auto [data_begin, data_end] = memory_begin_end<std::byte>(buffer_info);
  const auto begin = view.begin();
  const auto end = view.end();
  const auto view_begin = reinterpret_cast<const std::byte *>(&*begin);
  const auto view_end = reinterpret_cast<const std::byte *>(&*end);
  // Note the use of std::less, pointer comparison with operator< may be
  // undefined behavior with pointers from different arrays.
  return std::less<>()(data_begin, view_end) &&
         std::greater<>()(data_end, view_begin);
}

/// Copy all elements from src into dst.
/// Performs an explicit conversion of elements in `src` to the element type of
/// `dst` if `convert == true`.
/// Otherwise, elements in src are simply assigned to dst.
template <bool convert, class T, class View>
void copy_flattened(const py::array_t<T> &src, View &&dst) {
  if (scipp::size(dst) != src.size())
    throw std::runtime_error(
        "Numpy data size does not match size of target object.");

  [](const py::array_t<T> &src_, auto &&dst_) {
    const auto src_buffer = request_typed_buffer(src_);
    auto out = dst_.begin();
    copy_flattened<convert>(src_buffer, out);
  }(memory_overlaps(src, dst) ? py::array_t<T>(src.request()) : src,
    std::forward<View>(dst));
}

template <class SourceDType, class Destination>
void copy_array_into_view(const py::array_t<SourceDType> &src,
                          Destination &&dst, const Dimensions &dims) {
  const auto &shape = dims.shape();
  if (!std::equal(shape.begin(), shape.end(), src.shape(),
                  src.shape() + src.ndim()))
    throw except::DimensionError("The shape of the provided data "
                                 "does not match the existing "
                                 "object.");
  copy_flattened<ElementTypeMap<
      typename std::remove_reference_t<Destination>::value_type>::convert>(
      src, std::forward<Destination>(dst));
}

template <class SourceDType, class Destination>
void copy_array_into_view(const std::vector<SourceDType> &src,
                          Destination &&dst, const Dimensions &) {
  core::expect::sizeMatches(dst, src);
  std::copy(begin(src), end(src), dst.begin());
}

core::time_point make_time_point(const py::buffer &buffer, int64_t scale = 1);
