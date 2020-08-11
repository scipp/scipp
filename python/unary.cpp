// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <typename T> void bind_abs(py::module &m) {
  m.def(
      "abs", [](const typename T::const_view_type &x) { return abs(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "abs",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return abs(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_sqrt(py::module &m) {
  m.def(
      "sqrt", [](const typename T::const_view_type &x) { return sqrt(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "sqrt",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return sqrt(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_norm(py::module &m) {
  m.def(
      "norm", [](const typename T::const_view_type &x) { return norm(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_reciprocal(py::module &m) {
  m.def(
      "reciprocal",
      [](const typename T::const_view_type &x) { return reciprocal(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());
  m.def(
      "reciprocal",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return reciprocal(x, out); },
      py::arg("x"), py::arg("out"), py::keep_alive<0, 2>(),
      py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_nan_to_num(py::module &m) {
  m.def(
      "nan_to_num",
      [](const typename T::const_view_type &x,
         const std::optional<VariableConstView> &nan,
         const std::optional<VariableConstView> &posinf,
         const std::optional<VariableConstView> &neginf) {
        Variable out(x);
        if (nan)
          nan_to_num(out, *nan, out);
        if (posinf)
          positive_inf_to_num(out, *posinf, out);
        if (neginf)
          negative_inf_to_num(out, *neginf, out);
        return out;
      },
      py::arg("x"), py::arg("nan") = std::optional<VariableConstView>(),
      py::arg("posinf") = std::optional<VariableConstView>(),
      py::arg("neginf") = std::optional<VariableConstView>(),
      py::call_guard<py::gil_scoped_release>());

  m.def(
      "nan_to_num",
      [](const typename T::const_view_type &x,
         const std::optional<VariableConstView> &nan,
         const std::optional<VariableConstView> &posinf,
         const std::optional<VariableConstView> &neginf,
         const typename T::view_type &out) {
        if (nan)
          nan_to_num(x, *nan, out);
        if (posinf)
          positive_inf_to_num(x, *posinf, out);
        if (neginf)
          negative_inf_to_num(x, *neginf, out);
        return out;
      },
      py::arg("x"), py::arg("nan") = std::optional<VariableConstView>(),
      py::arg("posinf") = std::optional<VariableConstView>(),
      py::arg("neginf") = std::optional<VariableConstView>(), py::arg("out"),
      py::keep_alive<0, 5>(), py::call_guard<py::gil_scoped_release>());
}

void init_unary(py::module &m) {
  bind_abs<Variable>(m);
  bind_sqrt<Variable>(m);
  bind_norm<Variable>(m);
  bind_reciprocal<Variable>(m);
  bind_nan_to_num<Variable>(m);
}
