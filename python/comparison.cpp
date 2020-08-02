// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/comparison.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> Docstring docstring_comparison(const std::string op) {
  return Docstring()
      .description(
          "Comparison returning the truth value of " + op +
          " element-wise.\n\nNote and warning: If one or both of the operators "
          "have variances (uncertainties) there are ignored silently, i.e., "
          "comparison is based exclusively on the values.")
      .raises("If the units of inputs are not the same, or if the dtypes of "
              "inputs are not double precision floats.")
      .returns("Booleans that are true if " + op + ".")
      .rtype<T>()
      .template param<T>("x", "Input left operand.")
      .template param<T>("y", "Input right operand.");
}

template <typename T> void bind_less(py::module &m) {
  m.def(
      "less",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return less(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x < y)").c_str());
}

template <typename T> void bind_greater(py::module &m) {
  m.def(
      "greater",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return greater(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x > y)").c_str());
}

template <typename T> void bind_less_equal(py::module &m) {
  m.def(
      "less_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return less_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x <= y)").c_str());
}

template <typename T> void bind_greater_equal(py::module &m) {
  m.def(
      "greater_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return greater_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x >= y)").c_str());
}

template <typename T> void bind_equal(py::module &m) {
  m.def(
      "equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x == y)").c_str());
}

template <typename T> void bind_not_equal(py::module &m) {
  m.def(
      "not_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return not_equal(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      docstring_comparison<T>("(x != y)").c_str());
}

template <typename... Args> struct Inserter;

template <typename Arg, typename... Args> struct Inserter<Arg, Args...> {
  template <typename Map, typename T>
  static void insert(Map &map, const T &x, const T &y) {
    map[dtype<Arg>] = [&x, &y](const py::object &obj) {
      auto tol =
          pybind11::cast<Arg>(obj); // Will throw if cast does not succeed
      return is_approx(x, y, tol);
    };
    Inserter<Args...>::insert(map, x, y);
  }
};

template <> struct Inserter<> {
  template <typename Map, typename T> static void insert(Map &, T &, T &) {}
};

template <typename T, typename... Args>
std::map<DType, std::function<bool(const py::object &)>>
make_function_map(const T &x, const T &y) {
  std::map<DType, std::function<bool(const py::object &)>> funcmap;
  Inserter<Args...>::insert(funcmap, x, y);
  return funcmap;
}

template <class T> void bind_is_approx(py::module &m) {
  m.def(
      "is_approx",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y, const py::object &tol) {
        // Allowed types in following should come from (or be cross-checked
        // source records from INSTANTIATE_VARIABLE
        auto executor = make_function_map<typename T::const_view_type, int,
                                          int64_t, double, float>(x, y);
        if (executor.find(x.dtype()) == executor.end())
          throw scipp::except::TypeError(
              "is_approx does not support inputs of type " +
              to_string(x.dtype()));
        return executor[x.dtype()](tol);
      },
      py::arg("x"), py::arg("y"), py::arg("tol"),
      py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("is_approx determine if all values (and variances if "
                       "present) fall within the specified tolerance")
          .raises("if the dtype of input arguments (including tolerance) are "
                  "different")
          .returns("True if all values within tolerance.")
          .rtype<T>()
          .template param<T>("x", "Input left operand.")
          .template param<T>("y", "Input right operand.")
          .template param<T>("tol", "tolerance for comparison")
          .c_str());
}

template <typename T> void bind_is_equal(py::module &m) {
  m.def(
      "is_equal",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return x == y; },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("Variable level equals. Returns True if x and y "
                       "considered equal.")
          .returns("True if x == y")
          .rtype<T>()
          .template param<T>("x", "Input left operand.")
          .template param<T>("y", "Input right operand.")
          .c_str());
}

void init_comparison(py::module &m) {
  bind_less<Variable>(m);
  bind_greater<Variable>(m);
  bind_less_equal<Variable>(m);
  bind_greater_equal<Variable>(m);
  bind_equal<Variable>(m);
  bind_not_equal<Variable>(m);
  bind_is_approx<Variable>(m);
  bind_is_equal<Variable>(m);
}
