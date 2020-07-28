// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/reduction.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> void bind_flatten(py::module &m) {
  m.def("flatten",
        py::overload_cast<const typename T::const_view_type &, const Dim>(
            &flatten),
        py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        Docstring()
            .description("Flatten the specified dimension into event lists, "
                         "equivalent to summing dense data.")
            .raises("If the dimension does not exist, or if x does not contain "
                    "event lists.")
            .seealso(":py:func:`scipp.sum`")
            .returns("The flattened data.")
            .rtype<T>()
            .template param<T>("x", "Data container to flatten.")
            .param("dim", "Dimension over which to flatten.", "Dim")
            .c_str());
}

template <class T> Docstring docstring_mean() {
  return Docstring()
      .description(R"(
Element-wise mean over the specified dimension, if variances are present,
the new variance is computated as standard-deviation of the mean.

If the input has variances, the variances stored in the ouput are based on
the "standard deviation of the mean", i.e.,
:math:`\sigma_{mean} = \sigma / \sqrt{N}`.
:math:`N` is the length of the input dimension.
:math:`sigma` is estimated as the average of the standard deviations of
the input elements along that dimension.

This assumes that elements follow a normal distribution.)")
      .raises("If the dimension does not exist, or the dtype cannot be summed, "
              "e.g., if it is a string.")
      .returns("The mean of the input values.")
      .rtype<T>()
      .template param<T>("x", "Data to calculate mean of.")
      .param("dim", "Dimension along which to calculate the mean.", "Dim");
}

template <class T> void bind_mean(py::module &m) {
  m.def(
      "mean",
      [](const typename T::const_view_type &x, const Dim dim) {
        return mean(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_mean<T>().c_str());
}

template <class T> void bind_mean_out(py::module &m) {
  m.def(
      "mean",
      [](const typename T::const_view_type &x, const Dim dim,
         typename T::view_type out) { return mean(x, dim, out); },
      py::arg("x"), py::arg("dim"), py::arg("out"),
      py::call_guard<py::gil_scoped_release>(),
      docstring_mean<typename T::view_type>()
          .template param<T>("out", "Output buffer.")
          .c_str());
}

template <class T> Docstring docstring_sum() {
  return Docstring()
      .description("Element-wise sum over the specified dimension.")
      .raises("If the dimension does not exist, or the dtype cannot be summed, "
              "e.g., if it is a string.")
      .returns("The sum of the input values.")
      .rtype<T>()
      .template param<T>("x", "Data to calculate sum of.")
      .param("dim", "Dimension along which to calculate the sum.", "Dim");
}

template <class T> void bind_sum(py::module &m) {
  m.def(
      "sum",
      [](const typename T::const_view_type &x, const Dim dim) {
        return sum(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_sum<T>().c_str());
}

template <class T> void bind_sum_out(py::module &m) {
  m.def(
      "sum",
      [](const typename T::const_view_type &x, const Dim dim,
         const typename T::view_type &out) { return sum(x, dim, out); },
      py::arg("x"), py::arg("dim"), py::arg("out"),
      py::call_guard<py::gil_scoped_release>(),
      docstring_sum<typename T::view_type>()
          .template param<T>("out", "Output buffer.")
          .c_str());
}

template <class T> Docstring docstring_minmax(const std::string minmax) {
  return Docstring()
      .description("Element-wise " + minmax +
                   " over all of the input's dimensions.")
      .raises("If the dtype has no " + minmax + ", e.g., if it is a string.")
      .seealso(std::string(":py:func:`scipp.") +
               (minmax == "min" ? "max" : "min") + "`")
      .returns("The " + minmax + " of the input values.")
      .rtype<T>()
      .template param<T>("x", "Data to calculate " + minmax + " of.");
}

template <class T> void bind_min(py::module &m) {
  auto doc = docstring_minmax<T>("min");
  m.def(
      "min", [](const typename T::const_view_type &x) { return min(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "min",
      [](const typename T::const_view_type &x, const Dim dim) {
        return min(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      doc.description("Element-wise min over the specified dimension.")
          .raises(" If the dimension does not exist.", true)
          .param("dim", "Dimension over which to calculate the min.", "Dim")
          .c_str());
}

template <class T> void bind_max(py::module &m) {
  auto doc = docstring_minmax<T>("max");
  m.def(
      "max", [](const typename T::const_view_type &x) { return max(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(), doc.c_str());
  m.def(
      "max",
      [](const typename T::const_view_type &x, const Dim dim) {
        return max(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      doc.description("Element-wise min over the specified dimension.")
          .raises(" If the dimension does not exist", true)
          .param("dim", "Dimension over which to calculate the min.", "Dim")
          .c_str());
}

template <class T> Docstring docstring_bool(const std::string op) {
  return Docstring()
      .description(
          "Element-wise " + op +
          " over the specified dimension or all dimensions if not provided.")
      .raises("If the input dimension to reduce (optional) does not exist, or "
              "if the dtype is not bool.")
      .returns("The " + op + " combination of the input values.")
      .rtype<T>()
      .template param<T>("x", "Data to reduce.")
      .param("dim", "Dimension to reduce (optional).", "Dim");
}

template <class T> void bind_all(py::module &m) {
  m.def(
      "all", [](const typename T::const_view_type &x) { return all(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      docstring_bool<T>("AND").c_str());
  m.def(
      "all",
      [](const typename T::const_view_type &x, const Dim dim) {
        return all(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_bool<T>("AND").c_str());
}

template <class T> void bind_any(py::module &m) {
  m.def(
      "any", [](const typename T::const_view_type &x) { return any(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      docstring_bool<T>("OR").c_str());
  m.def(
      "any",
      [](const typename T::const_view_type &x, const Dim dim) {
        return any(x, dim);
      },
      py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_bool<T>("OR").c_str());
}

void init_reduction(py::module &m) {
  bind_flatten<Variable>(m);
  bind_flatten<DataArray>(m);
  bind_flatten<Dataset>(m);

  bind_mean<Variable>(m);
  bind_mean<DataArray>(m);
  bind_mean<Dataset>(m);
  bind_mean_out<Variable>(m);

  bind_sum<Variable>(m);
  bind_sum<DataArray>(m);
  bind_sum<Dataset>(m);
  bind_sum_out<Variable>(m);

  bind_min<Variable>(m);

  bind_max<Variable>(m);

  bind_all<Variable>(m);

  bind_any<Variable>(m);
}
