// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/sort.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/slice.h"
#include "scipp/variable/sort.h"
#include "scipp/variable/util.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

auto get_sort_order(const std::string &order) {
  if (order == "ascending")
    return variable::SortOrder::Ascending;
  else if (order == "descending")
    return variable::SortOrder::Descending;
  else
    throw std::runtime_error("Sort order must be 'ascending' or 'descending'");
}

template <typename T> void bind_dot(py::module &m) {
  auto doc = Docstring()
                 .description("Element-wise dot product.")
                 .raises("If the dtype of the input is not vector_3_float64.")
                 .returns("The dot product of the input vectors.")
                 .rtype<T>()
                 .template param<T>("x", "Input left hand side operand.")
                 .template param<T>("y", "Input right hand side operand.");
  m.def(
      "dot",
      [](const typename T::const_view_type &x,
         const typename T::const_view_type &y) { return dot(x, y); },
      py::arg("x"), py::arg("y"), py::call_guard<py::gil_scoped_release>(),
      doc.c_str());
}

template <typename T> void bind_sort(py::module &m) {
  m.def(
      "sort",
      [](const typename T::const_view_type &x,
         const typename Variable::const_view_type &key,
         const std::string &order) {
        return sort(x, key, get_sort_order(order));
      },
      py::arg("x"), py::arg("key"), py::arg("order"),
      py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_sort_dim(py::module &m) {
  m.def(
      "sort",
      [](const typename T::const_view_type &x, const Dim &dim,
         const std::string &order) {
        return sort(x, dim, get_sort_order(order));
      },
      py::arg("x"), py::arg("key"), py::arg("order"),
      py::call_guard<py::gil_scoped_release>());
}

void bind_is_sorted(py::module &m) {
  m.def(
      "is_sorted",
      [](const VariableConstView &x, const Dim dim, const std::string &order) {
        return is_sorted(x, dim, get_sort_order(order));
      },
      py::arg("x"), py::arg("dim"), py::arg("order") = "ascending",
      py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("Check if the values of a variable are sorted in.\n\nIf "
                       "'order' is 'ascending' checks if values are "
                       "non-decreasing along 'dim'. If 'order' is 'descending' "
                       "checks if values are non-increasing along 'dim'.")
          .param("x", "Variable to check.", "Variable")
          .param("dim", "Dimension along which order is checked.", "Dim")
          .param("order",
                 "Sorted order. Valid options are 'ascending' and "
                 "'descending'. Default is 'ascending'.",
                 "str")
          .returns(
              "True if the variable values are monotonously ascending or "
              "descending (depending on the requested order), False otherwise.")
          .rtype("bool")
          .c_str());
}

template <typename T> void bind_sort_variable(py::module &m) {
  m.def(
      "sort",
      [](const typename T::const_view_type &x, const Dim dim,
         const std::string &order) {
        return sort(x, dim, get_sort_order(order));
      },
      py::arg("x"), py::arg("key"), py::arg("order") = "ascending",
      py::call_guard<py::gil_scoped_release>());
}

void init_operations(py::module &m) {
  bind_dot<Variable>(m);

  bind_sort<Variable>(m);
  bind_sort<DataArray>(m);
  bind_sort<Dataset>(m);
  bind_sort_dim<DataArray>(m);
  bind_sort_dim<Dataset>(m);

  bind_sort_variable<Variable>(m);
  bind_is_sorted(m);

  m.def("values", variable::values,
        Docstring()
            .description("Return the variable without variances.")
            .seealso(":py:func:`scipp.variances`")
            .c_str(),
        py::call_guard<py::gil_scoped_release>());
  m.def("variances", variable::variances,
        Docstring()
            .description("Return variable containing the variances of the "
                         "input as values.")
            .seealso(":py:func:`scipp.values`")
            .c_str(),
        py::call_guard<py::gil_scoped_release>());

  m.def("get_slice_params",
        [](const VariableConstView &var, const VariableConstView &coord,
           const VariableConstView &begin, const VariableConstView &end) {
          const auto [dim, start, stop] =
              get_slice_params(var.dims(), coord, begin, end);
          // Do NOT release GIL since using py::slice
          return std::tuple{dim.name(), py::slice(start, stop, 1)};
        });
}
