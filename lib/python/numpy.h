// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cstddef>
#include <functional>
#include <sstream>
#include <vector>

#include "scipp/common/index_composition.h"
#include "scipp/core/parallel.h"
#include "scipp/variable/variable.h"

#include "nanobind.h"
#include "py_object.h"

using namespace scipp;

/// Return the numpy dtype object corresponding to the C++ element type.
template <class T> nb::object np_dtype_of() {
  constexpr const char *name = [] {
    if constexpr (std::is_same_v<T, double>)
      return "float64";
    else if constexpr (std::is_same_v<T, float>)
      return "float32";
    else if constexpr (std::is_same_v<T, int64_t>)
      return "int64";
    else if constexpr (std::is_same_v<T, int32_t>)
      return "int32";
    else if constexpr (std::is_same_v<T, bool>)
      return "bool";
    else
      static_assert(!sizeof(T), "No numpy dtype known for this type.");
  }();
  // Cached as an intentionally leaked handle: numpy dtype objects for builtin
  // types are immortal singletons and this runs on every values/variances
  // assignment. (A `static nb::object` would decref during static destruction,
  // potentially after interpreter shutdown.)
  static const nb::handle dt =
      nb::module_::import_("numpy").attr("dtype")(name).release();
  return nb::borrow<nb::object>(dt);
}

/// Wrapper around a numpy array of element type T. Provides the subset of the
/// pybind11 py::array_t API that the element-copy machinery below needs.
/// Holds both the Python object (for operations such as `copy`) and an
/// nb::ndarray referencing its memory (nanobind uses *element* strides).
template <class T> class py_array_t {
public:
  explicit py_array_t(nb::object obj) : m_obj(std::move(obj)) {
    if (!nb::try_cast(m_obj, m_array, false)) {
      // nb::ndarray uses element strides and cannot represent arrays whose
      // byte strides are not a multiple of the itemsize, e.g. field views
      // into structured arrays. Fall back to a (contiguous) copy, like
      // pybind11's forcecast did.
      PyErr_Clear(); // the failed cast may leave the error indicator set
      m_obj = m_obj.attr("copy")();
      m_array = nb::cast<nb::ndarray<const T, nb::numpy>>(m_obj, false);
    }
  }

  [[nodiscard]] scipp::index ndim() const {
    return static_cast<scipp::index>(m_array.ndim());
  }
  [[nodiscard]] scipp::index size() const {
    return static_cast<scipp::index>(m_array.size());
  }
  [[nodiscard]] scipp::index shape(const scipp::index i) const {
    return static_cast<scipp::index>(m_array.shape(i));
  }
  [[nodiscard]] scipp::index stride(const scipp::index i) const {
    return static_cast<scipp::index>(m_array.stride(i));
  }
  [[nodiscard]] const T *data() const { return m_array.data(); }
  [[nodiscard]] const nb::object &obj() const { return m_obj; }

  /// Element access with (element-based) stride arithmetic.
  template <class... Ix> const T &operator()(const Ix... index) const {
    scipp::index offset = 0;
    scipp::index d = 0;
    ((offset += index * stride(d++)), ...);
    return data()[offset];
  }

  [[nodiscard]] py_array_t copy() const {
    return py_array_t(m_obj.attr("copy")());
  }

private:
  nb::object m_obj;
  nb::ndarray<const T, nb::numpy> m_array;
};

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

/// Convert a nb::object referring to an array (or array-like nested
/// sequences) to a numpy array of the mapped element type if supported.
/// Otherwise, copies the contents into a std::vector<auto>.
template <class T>
auto cast_to_array_like(const nb::object &obj, const sc_units::Unit unit) {
  using TM = ElementTypeMap<T>;
  using PyType = typename TM::PyType;
  TM::check_assignable(obj, unit);
  if constexpr (std::is_same_v<T, core::time_point>) {
    // Convert datetime64 values to their int64 representation. `astype` is
    // needed because nb::ndarray cannot represent datetime64, and because
    // numpy.datetime64.__int__ delegates to datetime.datetime if the unit is
    // larger than ns and that cannot be converted to long.
    return py_array_t<PyType>(
        np_asarray(obj).attr("astype")(np_dtype_of<PyType>()));
  } else if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
    return py_array_t<PyType>(np_asarray(obj, np_dtype_of<PyType>()));
  } else {
    // nb::ndarray only supports POD types. Use a simple but expensive
    // solution for other types.
    // TODO Related to #290, we should properly support
    //  multi-dimensional input, and ignore bad shapes.
    try {
      return nb::cast<const std::vector<PyType>>(obj);
    } catch (const nb::cast_error &) {
      // The failed cast may leave the Python error indicator set, which
      // would make the calls below fail; we are raising our own error anyway.
      PyErr_Clear();
      // `obj` may be a plain (nested, possibly ragged) sequence without a
      // `dtype` attribute.
      std::string dtype;
      try {
        nb::object array = obj;
        if (!nb::hasattr(array, "dtype"))
          array = np_asarray(array);
        // Materialize as nb::object: passing the accessor to nb::str directly
        // would convert via the str type caster (and throw) instead of
        // calling str().
        const nb::object dt = array.attr("dtype");
        dtype = "dtype " + nb::cast<std::string>(nb::str(dt));
      } catch (const std::exception &) {
        dtype = "type " + nb::cast<std::string>(obj.type().attr("__name__"));
      }
      std::ostringstream oss;
      oss << "Unable to assign object of " << dtype << " to "
          << scipp::core::dtype<T>;
      throw std::invalid_argument(oss.str());
    }
  }
}

namespace scipp::detail {
namespace {
constexpr static size_t grainsize_1d = 10000;

template <class T> bool is_c_contiguous(const py_array_t<T> &array) {
  // Dimensions of size 1 are ignored, mirroring PyBUF_C_CONTIGUOUS (and
  // nanobind's own c_contig check). Kept as a hand-rolled loop: obtaining
  // the flag from nanobind would require a second ndarray cast per import,
  // which is more expensive than this O(ndim) loop.
  scipp::index expected = 1;
  for (scipp::index i = array.ndim() - 1; i >= 0; --i) {
    if (array.shape(i) != 1 && array.stride(i) != expected)
      return false;
    expected *= array.shape(i);
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
void copy_array_0d(const py_array_t<T> &src, Dst &dst) {
  auto it = dst.begin();
  copy_element<convert>(*src.data(), *it);
}

template <bool convert, class T, class Dst>
void copy_array_1d(const py_array_t<T> &src, Dst &dst) {
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
void copy_array_2d(const py_array_t<T> &src, Dst &dst) {
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * src.shape(1);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < src.shape(1); ++j, ++it)
            copy_element<convert>(src(i, j), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_array_3d(const py_array_t<T> &src, Dst &dst) {
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * src.shape(1) * src.shape(2);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < src.shape(1); ++j)
            for (scipp::index k = 0; k < src.shape(2); ++k, ++it)
              copy_element<convert>(src(i, j, k), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_array_4d(const py_array_t<T> &src, Dst &dst) {
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it =
            begin + range.begin() * src.shape(1) * src.shape(2) * src.shape(3);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < src.shape(1); ++j)
            for (scipp::index k = 0; k < src.shape(2); ++k)
              for (scipp::index l = 0; l < src.shape(3); ++l, ++it)
                copy_element<convert>(src(i, j, k, l), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_array_5d(const py_array_t<T> &src, Dst &dst) {
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * src.shape(1) * src.shape(2) *
                              src.shape(3) * src.shape(4);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < src.shape(1); ++j)
            for (scipp::index k = 0; k < src.shape(2); ++k)
              for (scipp::index l = 0; l < src.shape(3); ++l)
                for (scipp::index m = 0; m < src.shape(4); ++m, ++it)
                  copy_element<convert>(src(i, j, k, l, m), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_array_6d(const py_array_t<T> &src, Dst &dst) {
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src.shape(0)), [&](const auto &range) {
        auto it = begin + range.begin() * src.shape(1) * src.shape(2) *
                              src.shape(3) * src.shape(4) * src.shape(5);
        for (scipp::index i = range.begin(); i < range.end(); ++i)
          for (scipp::index j = 0; j < src.shape(1); ++j)
            for (scipp::index k = 0; k < src.shape(2); ++k)
              for (scipp::index l = 0; l < src.shape(3); ++l)
                for (scipp::index m = 0; m < src.shape(4); ++m)
                  for (scipp::index n = 0; n < src.shape(5); ++n, ++it)
                    copy_element<convert>(src(i, j, k, l, m, n), *it);
      });
}

template <bool convert, class T, class Dst>
void copy_flattened(const py_array_t<T> &src_array, Dst &dst) {
  const auto *src = src_array.data();
  const auto begin = dst.begin();
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, src_array.size(), grainsize_1d),
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
  const auto ndim = array.ndim();
  std::vector<scipp::index> shape(ndim);
  std::vector<scipp::index> byte_strides(ndim);
  for (scipp::index i = 0; i < ndim; ++i) {
    shape[i] = array.shape(i);
    byte_strides[i] = array.stride(i) * static_cast<scipp::index>(sizeof(T));
  }
  const auto [begin_offset, end_offset] =
      memory_bounds(shape.begin(), shape.end(), byte_strides.begin());
  return std::pair{begin + begin_offset, end + end_offset};
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
  if (scipp::size(dst) != src.size())
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
  dispatch(memory_overlaps(src, dst) ? src.copy() : src);
}
} // namespace
} // namespace scipp::detail

template <class SourceDType, class Destination>
void copy_array_into_view(const py_array_t<SourceDType> &src, Destination &&dst,
                          const Dimensions &dims) {
  const auto &shape = dims.shape();
  const auto shape_matches = [&] {
    if (static_cast<scipp::index>(shape.size()) != src.ndim())
      return false;
    for (scipp::index i = 0; i < src.ndim(); ++i)
      if (shape[i] != src.shape(i))
        return false;
    return true;
  };
  if (!shape_matches())
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
