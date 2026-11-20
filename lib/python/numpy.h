// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "scipp/common/index_composition.h"
#include "scipp/core/parallel.h"
#include "scipp/variable/variable.h"

#include "nanobind.h"
#include "numpy_c_api.h"
#include "py_object.h"

namespace nb = nanobind;

using namespace scipp;

/// Typed view over a numpy array; strides are in elements, not bytes.
template <class T>
using py_array_t = nb::ndarray<const T, nb::numpy, nb::device::cpu>;

/// Map C++ types to Python types to perform conversion between scipp containers
/// and numpy arrays.
template <class T> struct ElementTypeMap {
  using PyType = T;
  constexpr static bool convert = false;

  static void check_assignable(const nb::object &, const sc_units::Unit &) {}
};

template <> struct ElementTypeMap<scipp::core::time_point> {
  using PyType = int64_t;
  constexpr static bool convert = true;

  static void check_assignable(const nb::object &obj, sc_units::Unit unit);
};

template <> struct ElementTypeMap<scipp::python::PyObject> {
  using PyType = nb::object;
  constexpr static bool convert = true;

  static void check_assignable(const nb::object &, const sc_units::Unit &) {}
};

/// Cast a nb::object referring to an array to a typed ndarray if supported.
/// Otherwise, copies the contents into a std::vector<auto>.
template <class T>
auto cast_to_array_like(const nb::object &obj, const sc_units::Unit unit) {
  using TM = ElementTypeMap<T>;
  using PyType = typename TM::PyType;
  TM::check_assignable(obj, unit);
  if constexpr (std::is_same_v<T, core::time_point>) {
    // Convert to int64 via astype because numpy.datetime64.__int__
    // delegates to datetime.datetime if the unit is larger than ns and
    // that cannot be converted to long. astype returns a (C-contiguous)
    // copy which the ndarray caster keeps alive.
    return nb::cast<py_array_t<PyType>>(
        scipp::python::as_numpy_array(obj).attr("astype")("int64"));
  } else if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
    // Other buffer exporters may omit strides. Let NumPy normalize those and
    // packed fields whose byte strides cannot be expressed in elements.
    if (py_array_t<PyType> array;
        PyArray_Check(obj.ptr()) &&
        PyArray_ISALIGNED(reinterpret_cast<const PyArrayObject *>(obj.ptr())) &&
        nb::try_cast(obj, array, /*convert=*/false) &&
        reinterpret_cast<std::uintptr_t>(array.data()) % alignof(PyType) == 0)
      return array;
    return nb::cast<py_array_t<PyType>>(scipp::python::as_numpy_array(
        obj, scipp::python::numpy_typenum<PyType>(), NPY_ARRAY_CARRAY_RO));
  } else {
    // nb::ndarray only supports arithmetic dtypes. Use a simple but expensive
    // solution for other types.
    // TODO Related to #290, we should properly support
    //  multi-dimensional input, and ignore bad shapes.
    // pybind11 caught both of these as std::runtime_error.
    std::optional<std::vector<PyType>> values;
    try {
      values = nb::cast<std::vector<PyType>>(obj);
    } catch (const nb::cast_error &) {
    } catch (const nb::python_error &) {
    }
    if (values)
      return std::move(*values);
    const nb::object dtype = scipp::python::as_numpy_array(obj).attr("dtype");
    std::ostringstream oss;
    oss << "Unable to assign object of dtype " << nb::str(dtype).c_str()
        << " to " << scipp::core::dtype<T>;
    throw std::invalid_argument(oss.str());
  }
}

namespace scipp::detail {
namespace {
constexpr static size_t grainsize_1d = 10000;

template <class T> bool is_c_contiguous(const py_array_t<T> &array) {
  // Strides of dimensions with extent 0 or 1 are arbitrary, ignore them.
  int64_t expected = 1;
  for (auto i = static_cast<int64_t>(array.ndim()); i-- > 0;) {
    if (static_cast<int64_t>(array.shape(i)) > 1 && array.stride(i) != expected)
      return false;
    expected *= static_cast<int64_t>(array.shape(i));
  }
  return true;
}

template <bool convert, class Source, class Destination>
void copy_element(const Source &src, Destination &&dst) {
  if constexpr (convert) {
    dst = std::remove_reference_t<Destination>{src};
  } else {
    std::forward<Destination>(dst) = src;
  }
}

template <bool convert, class T, class Dst>
void copy_array_0d(const py_array_t<T> &src_array, Dst &dst) {
  copy_element<convert>(*src_array.data(), *dst.begin());
}

template <bool convert, class T, class Dst>
void copy_array_1d(const py_array_t<T> &src_array, Dst &dst) {
  const auto src = src_array.template view<nb::ndim<1>>();
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0), grainsize_1d),
      [&](const auto &range) {
        auto it = begin + range.begin();
        for (scipp::index i = range.begin(); i < range.end(); ++i, ++it) {
          copy_element<convert>(src(i), *it);
        }
      });
}

template <bool convert, class T, class Dst>
void copy_array_2d(const py_array_t<T> &src_array, Dst &dst) {
  const auto src = src_array.template view<nb::ndim<2>>();
  const auto shape = [&src](const size_t i) {
    return static_cast<scipp::index>(src.shape(i));
  };
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * shape(1);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < shape(1); ++j, ++it)
            copy_element<convert>(src(i, j), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_array_3d(const py_array_t<T> &src_array, Dst &dst) {
  const auto src = src_array.template view<nb::ndim<3>>();
  const auto shape = [&src](const size_t i) {
    return static_cast<scipp::index>(src.shape(i));
  };
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * shape(1) * shape(2);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < shape(1); ++j)
            for (scipp::index k = 0; k < shape(2); ++k, ++it)
              copy_element<convert>(src(i, j, k), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_array_4d(const py_array_t<T> &src_array, Dst &dst) {
  const auto src = src_array.template view<nb::ndim<4>>();
  const auto shape = [&src](const size_t i) {
    return static_cast<scipp::index>(src.shape(i));
  };
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * shape(1) * shape(2) * shape(3);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < shape(1); ++j)
            for (scipp::index k = 0; k < shape(2); ++k)
              for (scipp::index l = 0; l < shape(3); ++l, ++it)
                copy_element<convert>(src(i, j, k, l), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_array_5d(const py_array_t<T> &src_array, Dst &dst) {
  const auto src = src_array.template view<nb::ndim<5>>();
  const auto shape = [&src](const size_t i) {
    return static_cast<scipp::index>(src.shape(i));
  };
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it =
            begin + range.begin() * shape(1) * shape(2) * shape(3) * shape(4);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < shape(1); ++j)
            for (scipp::index k = 0; k < shape(2); ++k)
              for (scipp::index l = 0; l < shape(3); ++l)
                for (scipp::index m = 0; m < shape(4); ++m, ++it)
                  copy_element<convert>(src(i, j, k, l, m), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_array_6d(const py_array_t<T> &src_array, Dst &dst) {
  const auto src = src_array.template view<nb::ndim<6>>();
  const auto shape = [&src](const size_t i) {
    return static_cast<scipp::index>(src.shape(i));
  };
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * shape(1) * shape(2) * shape(3) *
                              shape(4) * shape(5);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < shape(1); ++j)
            for (scipp::index k = 0; k < shape(2); ++k)
              for (scipp::index l = 0; l < shape(3); ++l)
                for (scipp::index m = 0; m < shape(4); ++m)
                  for (scipp::index n = 0; n < shape(5); ++n, ++it)
                    copy_element<convert>(src(i, j, k, l, m, n), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_flattened(const py_array_t<T> &src_array, Dst &dst) {
  const auto *src = src_array.data();
  const auto size = static_cast<scipp::index>(src_array.size());
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, size, grainsize_1d),
      [&](const auto &range) {
        auto it = begin + range.begin();
        for (scipp::index i = range.begin(); i < range.end(); ++i, ++it) {
          copy_element<convert>(src[i], *it);
        }
      });
}

template <class T> auto memory_begin_end(const py_array_t<T> &array) {
  const auto *begin = reinterpret_cast<const std::byte *>(array.data());
  const auto *end = begin;
  // memory_bounds computes offsets in stride units, i.e. elements here.
  const auto [begin_offset, end_offset] = memory_bounds(
      array.shape_ptr(), array.shape_ptr() + array.ndim(), array.stride_ptr());
  const auto itemsize = static_cast<int64_t>(array.itemsize());
  return std::pair{begin + begin_offset * itemsize,
                   end + end_offset * itemsize};
}

template <class T, class View>
bool memory_overlaps(const py_array_t<T> &data, const View &view) {
  const auto [data_begin, data_end] = memory_begin_end(data);
  const auto begin = view.begin();
  const auto end = view.end();
  const auto view_begin = reinterpret_cast<const std::byte *>(&*begin);
  const auto view_end = reinterpret_cast<const std::byte *>(&*end);
  // Note the use of std::less, pointer comparison with operator< may be
  // undefined behavior with pointers from different arrays.
  return std::less<>()(data_begin, view_end) &&
         std::greater<>()(data_end, view_begin);
}

/*
 * The code here is not pretty.
 * But a generic copy function would be much more complicated than the
 * straightforward nested loops we use here.
 * In practice, there is also little need to support ndim > 6 for non-contiguous
 * data as transform does not support such variables either.
 *
 * For a working, generic implementation, see git ref
 *  bd2e5f0a84d02bd5baf6d0afc32a2ab66dc09e2b
 * and its history, in particular
 *  86761b1e280a63b4f0b723a165188d21dd097972
 *  8721b2d02b98c1acae5c786ffda88055551d832b
 *  4c03a553827f2881672ae1f00f43ae06e879452c
 *  c2a1e3898467083bf7d019a3cb54702c8b50ba86
 *  c2a1e3898467083bf7d019a3cb54702c8b50ba86
 */
/// Copy all elements from src into dst.
/// Performs an explicit conversion of elements in `src` to the element type of
/// `dst` if `convert == true`.
/// Otherwise, elements in src are simply assigned to dst.
template <bool convert, class T, class Dst>
void copy_elements(const py_array_t<T> &src, Dst &dst) {
  if (scipp::size(dst) != static_cast<scipp::index>(src.size()))
    throw std::runtime_error(
        "Numpy data size does not match size of target object.");

  const auto dispatch = [&dst](const py_array_t<T> &src_) {
    if (is_c_contiguous(src_))
      return copy_flattened<convert>(src_, dst);

    switch (src_.ndim()) {
    case 0:
      return copy_array_0d<convert>(src_, dst);
    case 1:
      return copy_array_1d<convert>(src_, dst);
    case 2:
      return copy_array_2d<convert>(src_, dst);
    case 3:
      return copy_array_3d<convert>(src_, dst);
    case 4:
      return copy_array_4d<convert>(src_, dst);
    case 5:
      return copy_array_5d<convert>(src_, dst);
    case 6:
      return copy_array_6d<convert>(src_, dst);
    default:
      throw std::runtime_error(
          "Numpy array with non-c-contiguous memory layout has more "
          "dimensions than supported in the current implementation. "
          "Try making a copy of the array first to get a "
          "c-contiguous layout.");
    }
  };
  if (memory_overlaps(src, dst)) {
    const auto copy = scipp::python::as_numpy_array(
        nb::cast(src, nb::rv_policy::reference), NPY_NOTYPE,
        NPY_ARRAY_ENSURECOPY | NPY_ARRAY_C_CONTIGUOUS);
    copy_flattened<convert>(nb::cast<py_array_t<T>>(copy), dst);
  } else
    dispatch(src);
}
} // namespace
} // namespace scipp::detail

template <class SourceDType, class Destination>
void copy_array_into_view(const py_array_t<SourceDType> &src, Destination &&dst,
                          const Dimensions &dims) {
  const auto &shape = dims.shape();
  if (static_cast<size_t>(src.ndim()) != shape.size() ||
      !std::equal(shape.begin(), shape.end(), src.shape_ptr(),
                  src.shape_ptr() + src.ndim()))
    throw except::DimensionError("The shape of the provided data "
                                 "does not match the existing "
                                 "object.");
  scipp::detail::copy_elements<ElementTypeMap<
      typename std::remove_reference_t<Destination>::value_type>::convert>(src,
                                                                           dst);
}

template <class SourceDType, class Destination>
void copy_array_into_view(const std::vector<SourceDType> &src, Destination &dst,
                          const Dimensions &) {
  core::expect::sizeMatches(dst, src);
  std::copy(begin(src), end(src), dst.begin());
}

core::time_point make_time_point(const nb::object &buffer, int64_t scale = 1);
