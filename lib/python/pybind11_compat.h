// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
#pragma once

// nanobind's casters for the types below accept fewer Python arguments than
// pybind11's, which Scipp's bindings were written against. These casters
// follow pybind11 3.0's conversion rules, so that switching the binding
// library does not change which arguments Scipp accepts. Include this header
// via "nanobind.h" only, before any other nanobind header uses these types.

#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/detail/nb_list.h>
#include <nanobind/stl/detail/nb_optional.h>

namespace scipp::python {
/// nanobind's bool caster is final and only accepts True and False.
struct bool_arg {
  bool value;
  operator bool() const noexcept { return value; }
};
} // namespace scipp::python

namespace nanobind::detail {

/// Accepts NumPy bools, and when converting also None and objects
/// implementing __bool__.
template <> struct type_caster<scipp::python::bool_arg> {
  NB_TYPE_CASTER(scipp::python::bool_arg, const_name("bool"))

  bool from_python(handle src, uint32_t flags, cleanup_list *) noexcept {
    if (src.ptr() == Py_True || src.ptr() == Py_False) {
      value = {src.ptr() == Py_True};
      return true;
    }
    const char *name = Py_TYPE(src.ptr())->tp_name;
    if (!(flags & cast_flags::convert) && std::strcmp(name, "numpy.bool") &&
        std::strcmp(name, "numpy.bool_"))
      return false;
    if (src.is_none()) {
      value = {false};
      return true;
    }
    if (PyType_GetSlot(Py_TYPE(src.ptr()), Py_nb_bool)) {
      if (const int truth = PyObject_IsTrue(src.ptr()); truth >= 0) {
        value = {truth != 0};
        return true;
      }
      PyErr_Clear();
    }
    return false;
  }
  static handle from_cpp(scipp::python::bool_arg src, rv_policy,
                         cleanup_list *) noexcept {
    return handle(src.value ? Py_True : Py_False).inc_ref();
  }
};

/// Accepts int and objects implementing __index__, and when converting also
/// numbers implementing __int__. Rejects float.
template <class T> struct pybind11_int_caster {
  NB_TYPE_CASTER(T, const_name("int"))

  bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
    const bool convert = flags & cast_flags::convert;
    if (PyFloat_Check(src.ptr()) ||
        (!convert && !PyLong_Check(src.ptr()) && !PyIndex_Check(src.ptr())))
      return false;
    const long long result = PyLong_AsLongLong(src.ptr());
    const bool error = result == -1 && PyErr_Occurred();
    if (error || result != static_cast<long long>(static_cast<T>(result))) {
      PyErr_Clear();
      if (error && convert && PyNumber_Check(src.ptr())) {
        const auto integer = steal(PyNumber_Long(src.ptr()));
        PyErr_Clear();
        return integer.is_valid() &&
               from_python(integer, flags & ~cast_flags::convert, cleanup);
      }
      return false;
    }
    value = static_cast<T>(result);
    return true;
  }
  static handle from_cpp(T src, rv_policy, cleanup_list *) noexcept {
    return PyLong_FromLongLong(src);
  }
};

template <> struct type_caster<int> : pybind11_int_caster<int> {};
template <> struct type_caster<long> : pybind11_int_caster<long> {};
template <> struct type_caster<long long> : pybind11_int_caster<long long> {};

/// Accepts float subclasses, and when converting also any number.
template <> struct type_caster<double> {
  NB_TYPE_CASTER(double, const_name("float"))

  bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
    const bool convert = flags & cast_flags::convert;
    if (!convert && !PyFloat_Check(src.ptr()))
      return false;
    value = PyFloat_AsDouble(src.ptr());
    if (value == -1.0 && PyErr_Occurred()) {
      PyErr_Clear();
      if (convert && PyNumber_Check(src.ptr())) {
        const auto real = steal(PyNumber_Float(src.ptr()));
        PyErr_Clear();
        return real.is_valid() &&
               from_python(real, flags & ~cast_flags::convert, cleanup);
      }
      return false;
    }
    return true;
  }
  static handle from_cpp(double src, rv_policy, cleanup_list *) noexcept {
    return PyFloat_FromDouble(src);
  }
};

/// Also accepts bytes and bytearray without decoding.
template <class String> struct pybind11_string_caster {
  NB_TYPE_CASTER(String, const_name("str"))

  bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
    const char *data;
    Py_ssize_t size;
    if (PyUnicode_Check(src.ptr())) {
      if (!(data = PyUnicode_AsUTF8AndSize(src.ptr(), &size))) {
        PyErr_Clear();
        return false;
      }
    } else if (PyBytes_Check(src.ptr())) {
      data = PyBytes_AS_STRING(src.ptr());
      size = PyBytes_GET_SIZE(src.ptr());
    } else if (PyByteArray_Check(src.ptr())) {
      // Copy under a lock, since another thread may resize the bytearray.
      ft_object_guard guard(src);
      bytearray.assign(PyByteArray_AS_STRING(src.ptr()),
                       static_cast<size_t>(PyByteArray_GET_SIZE(src.ptr())));
      data = bytearray.data();
      size = static_cast<Py_ssize_t>(bytearray.size());
    } else {
      return false;
    }
    value = String(data, static_cast<size_t>(size));
    return true;
  }
  static handle from_cpp(const String &src, rv_policy,
                         cleanup_list *) noexcept {
    return PyUnicode_FromStringAndSize(src.data(),
                                       static_cast<Py_ssize_t>(src.size()));
  }

private:
  /// Owns the characters of a bytearray, which a std::string_view refers to.
  std::string bytearray;
};

template <>
struct type_caster<std::string> : pybind11_string_caster<std::string> {};
template <>
struct type_caster<std::string_view>
    : pybind11_string_caster<std::string_view> {};

/// Accepts sequences other than str and bytes, and when converting also
/// generators, sets, dict views, map and zip. Errors raised by len() or by
/// iterating propagate, as with pybind11.
template <class Type, class Alloc>
struct type_caster<std::vector<Type, Alloc>>
    : list_caster<std::vector<Type, Alloc>, Type> {
  bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) {
    object seq = borrow(src);
    if (PySequence_Check(src.ptr())) {
      if (PyUnicode_Check(src.ptr()) || PyBytes_Check(src.ptr()))
        return false;
    } else if (PyGen_Check(src.ptr()) || PyAnySet_Check(src.ptr()) ||
               is_iterable_view(src)) {
      if (!(flags & cast_flags::convert))
        return false;
      seq = steal(PySequence_Tuple(src.ptr()));
      if (!seq.is_valid())
        throw python_error();
    } else {
      return false;
    }
    const Py_ssize_t size = PySequence_Size(seq.ptr());
    if (size < 0)
      throw python_error();
    this->value.clear();
    this->value.reserve(static_cast<size_t>(size));
    for (handle item : seq) {
      make_caster<Type> caster;
      if (!caster.from_python(item, flags_for_local_caster<Type>(flags),
                              cleanup) ||
          !caster.template can_cast<Type>())
        return false;
      this->value.push_back(caster.operator cast_t<Type>());
    }
    return true;
  }

private:
  static bool is_iterable_view(handle src) noexcept {
    if (PyType_Check(src.ptr()))
      return false;
    const char *name = Py_TYPE(src.ptr())->tp_name;
    for (const char *view :
         {"dict_keys", "dict_values", "dict_items", "map", "zip"})
      if (std::strcmp(name, view) == 0)
        return true;
    return false;
  }
};

/// nanobind's optional caster is noexcept, which would turn errors raised
/// while loading the vector into std::terminate.
template <class Type, class Alloc>
struct type_caster<std::optional<std::vector<Type, Alloc>>>
    : optional_caster<std::optional<std::vector<Type, Alloc>>> {
  bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) {
    if (src.is_none()) {
      this->value.reset();
      return true;
    }
    make_caster<std::vector<Type, Alloc>> caster;
    if (!caster.from_python(src, flags, cleanup))
      return false;
    this->value.emplace(std::move(caster.value));
    return true;
  }
};

/// Accepts any sequence of matching length and reads its items by position.
/// Errors raised by len() or by reading an item propagate, as with pybind11.
template <class... Ts> struct type_caster<std::tuple<Ts...>> {
  static constexpr size_t N = sizeof...(Ts);
  using Value = std::tuple<Ts...>;
  using Indices = std::make_index_sequence<N>;
  static constexpr auto Name = const_name("tuple[") + const_name < N ==
                               0 > (const_name("()"),
                                    concat(make_caster<Ts>::Name...)) +
                                       const_name("]");
  template <typename T> using Cast = Value;

  bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) {
    if (!PySequence_Check(src.ptr()))
      return false;
    const Py_ssize_t size = PySequence_Size(src.ptr());
    if (size < 0)
      throw python_error();
    return static_cast<size_t>(size) == N &&
           from_python_impl(src, flags, cleanup, Indices{});
  }

  template <typename T>
  static handle from_cpp(T &&value, rv_policy policy, cleanup_list *cleanup) {
    return from_cpp_impl(std::forward<T>(value), policy, cleanup, Indices{});
  }

  template <typename T> bool can_cast() const noexcept {
    return can_cast_impl(Indices{});
  }

  explicit operator Value() { return cast_impl(Indices{}); }

private:
  template <size_t... Is>
  bool from_python_impl(handle src, uint32_t flags, cleanup_list *cleanup,
                        std::index_sequence<Is...>) {
    static_cast<void>(src);
    static_cast<void>(flags);
    static_cast<void>(cleanup);
    // Like pybind11, read each item only after the previous one loaded.
    return (... && load_item<Is, Ts>(src, flags, cleanup));
  }

  template <size_t I, class T>
  bool load_item(handle src, uint32_t flags, cleanup_list *cleanup) {
    // Keep the item alive for casters that refer into it.
    items[I] = steal(PySequence_GetItem(src.ptr(), static_cast<Py_ssize_t>(I)));
    if (!items[I].is_valid())
      throw python_error();
    return std::get<I>(casters).from_python(
        items[I], flags_for_local_caster<T>(flags), cleanup);
  }

  template <typename T, size_t... Is>
  static handle from_cpp_impl(T &&value, rv_policy policy,
                              cleanup_list *cleanup,
                              std::index_sequence<Is...>) {
    static_cast<void>(policy);
    static_cast<void>(cleanup);
    PyObject *items[N > 0 ? N : 1]{};
    const bool success =
        (... && ((items[Is] = make_caster<Ts>::from_cpp(
                                  forward_like_<T>(std::get<Is>(value)), policy,
                                  cleanup)
                                  .ptr()) != nullptr));
    PyObject *result = success ? PyTuple_New(N) : nullptr;
    for (size_t i = 0; i < N; ++i) {
      if (result)
        PyTuple_SET_ITEM(result, i, items[i]);
      else
        Py_XDECREF(items[i]);
    }
    return result;
  }

  template <size_t... Is>
  bool can_cast_impl(std::index_sequence<Is...>) const noexcept {
    return (std::get<Is>(casters).template can_cast<Ts>() && ...);
  }
  template <size_t... Is> Value cast_impl(std::index_sequence<Is...>) {
    return Value(std::get<Is>(casters).operator cast_t<Ts>()...);
  }

  std::tuple<make_caster<Ts>...> casters;
  std::array<object, N> items;
};

} // namespace nanobind::detail
