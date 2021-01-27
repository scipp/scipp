// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/groupby.h"
#include "scipp/dataset/dataset.h"

#include "docstring.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> Docstring docstring_groupby(const std::string &op) {
  return Docstring()
      .description("Element-wise " + op +
                   " over the specified dimension "
                   "within a group.")
      .returns("The computed " + op +
               " over each group, combined along "
               "the dimension specified when calling :py:func:`scipp.groupby`.")
      .rtype<T>()
      .param("dim", "Dimension to reduce when computing the " + op + ".",
             "Dim");
}

template <class T> void bind_groupby(py::module &m, const std::string &name) {
  m.def("groupby",
        py::overload_cast<const typename T::const_view_type &, const Dim>(
            &groupby),
        py::arg("data"), py::arg("group"),
        py::call_guard<py::gil_scoped_release>());

  m.def("groupby",
        py::overload_cast<const typename T::const_view_type &, const Dim,
                          const VariableConstView &>(&groupby),
        py::arg("data"), py::arg("group"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>());

  m.def("groupby",
        py::overload_cast<const typename T::const_view_type &,
                          const VariableConstView &, const VariableConstView &>(
            &groupby),
        py::arg("data"), py::arg("group"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>());

  py::class_<GroupBy<T>> groupBy(m, name.c_str(), R"(
    GroupBy object implementing to split-apply-combine mechanism.)");

  groupBy.def("mean", &GroupBy<T>::mean, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(),
              docstring_groupby<T>("mean").c_str());

  groupBy.def("sum", &GroupBy<T>::sum, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(),
              docstring_groupby<T>("sum").c_str());

  groupBy.def("all", &GroupBy<T>::all, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(),
              docstring_groupby<T>("all").c_str());

  groupBy.def("any", &GroupBy<T>::any, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(),
              docstring_groupby<T>("any").c_str());

  groupBy.def("min", &GroupBy<T>::min, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(),
              docstring_groupby<T>("min").c_str());

  groupBy.def("max", &GroupBy<T>::max, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(),
              docstring_groupby<T>("max").c_str());

  groupBy.def("concatenate", &GroupBy<T>::concatenate, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(),
              docstring_groupby<T>("concatenate").c_str());

  groupBy.def(
      "copy",
      [](const GroupBy<T> &self, const scipp::index &group) {
        return self.copy(group);
      },
      py::arg("group"),
      Docstring()
          .description("Extract group as new data array or dataset.")
          .rtype<T>()
          .template param<scipp::index>("group", "Index of groupy to extract")
          .c_str());
}

void init_groupby(py::module &m) {
  bind_groupby<DataArray>(m, "GroupByDataArray");
  bind_groupby<Dataset>(m, "GroupByDataset");
}
