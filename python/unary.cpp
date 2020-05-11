// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <typename T> void bind_abs(py::module &m) {
  auto doc =
      Docstring()
          .description("Element-wise absolute value.")
          .raises(
              "If the dtype has no absolute value, e.g., if it is a string.")
          .seealso(":py:func:`scipp.norm` for vector-like dtype.")
          .returns("The absolute values of the input.")
          .rtype<T>()
          .template param<T>("x", "Input data.");
  m.def(
      "abs", [](const typename T::const_view_type &x) { return abs(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "abs",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return abs(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer.")
          .c_str());
}

template <typename T> void bind_sqrt(py::module &m) {
  auto doc =
      Docstring()
          .description("Element-wise square-root.")
          .raises("If the dtype has no square-root, e.g., if it is a string.")
          .returns("The square-root values of the input.")
          .rtype<T>()
          .template param<T>("x", "Input data.");
  m.def(
      "sqrt", [](const typename T::const_view_type &x) { return sqrt(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "sqrt",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return sqrt(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer.")
          .c_str());
}

template <typename T> void bind_norm(py::module &m) {
  auto doc =
      Docstring()
          .description("Element-wise norm.")
          .raises("If the dtype has no norm, i.e., if it is not a vector.")
          .returns("Scalar elements computed as the norm values of the input "
                   "elements.")
          .rtype<T>()
          .template param<T>("x", "Input data.");
  m.def(
      "norm", [](const typename T::const_view_type &x) { return norm(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
}

template <typename T> void bind_reciprocal(py::module &m) {
  auto doc =
      Docstring()
          .description("Element-wise reciprocal.")
          .raises("If the dtype has no reciprocal, e.g., if it is a string.")
          .returns("The reciprocal values of the input.")
          .rtype<T>()
          .template param<T>("x", "Input data.");
  m.def(
      "reciprocal",
      [](const typename T::const_view_type &x) { return reciprocal(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "reciprocal",
      [](const typename T::const_view_type &x,
         const typename T::view_type &out) { return reciprocal(x, out); },
      py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
      doc.template rtype<typename T::view_type>()
          .template param<T>("out", "Output buffer.")
          .c_str());
}

template <typename T> void bind_nan_to_num(py::module &m) {
  auto doc =
      Docstring()
          .description(
              R"(Element-wise special value replacement

All elements in the output are identical to input except in the presence
of a NaN, Inf or -Inf.
The function allows replacements to be separately specified for nan, inf
or -inf values.
You can choose to replace a subset of those special values by providing
just the required key word arguments.
If the replacement is value-only and the input has variances,
the variance at the element(s) undergoing replacement are also replaced
with the replacement value.
If the replacement has a variance and the input has variances,
the variance at the element(s) undergoing replacement are also replaced
with the replacement variance.)")
          .raises("If the types of input and replacement do not match.")
          .returns("Input elements are replaced in output with specified "
                   "subsitutions.")
          .rtype<T>()
          .template param<T>("x", "Input data.")
          .param("nan", "Replacement values for NaN in the input.", "Variable")
          .param("posinf", "Replacement values for Inf in the input.",
                 "Variable")
          .param("neginf", "Replacement values for -Inf in the input.",
                 "Variable");

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
      py::call_guard<py::gil_scoped_release>(), doc.c_str());

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
      py::call_guard<py::gil_scoped_release>(),
      doc.template param<T>("out", "Output buffer.")
          .template rtype<typename T::view_type>()
          .c_str());
}

void init_unary(py::module &m) {
  bind_abs<Variable>(m);
  bind_sqrt<Variable>(m);
  bind_norm<Variable>(m);
  bind_reciprocal<Variable>(m);
  bind_nan_to_num<Variable>(m);
}
