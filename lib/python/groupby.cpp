// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define BIND_GROUPBY_OP(CLS, NAME)                                             \
  CLS.def(                                                                     \
      TOSTRING(NAME),                                                          \
      [](const GroupBy<T> &self, const std::string &dim) {                     \
        return self.NAME(Dim{dim});                                            \
      },                                                                       \
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),                \
      docstring_groupby<T>(TOSTRING(NAME)).c_str());

template <class T> void bind_groupby(py::module &m, const std::string &name) {
  m.def(
      "groupby",
      [](const T &x, const std::string &dim) { return groupby(x, Dim{dim}); },
      py::arg("data"), py::arg("group"),
      py::call_guard<py::gil_scoped_release>());

  m.def(
      "groupby",
      [](const T &x, const std::string &dim, const Variable &bins) {
        return groupby(x, Dim{dim}, bins);
      },
      py::arg("data"), py::arg("group"), py::arg("bins"),
      py::call_guard<py::gil_scoped_release>());

  m.def("groupby",
        py::overload_cast<const T &, const Variable &, const Variable &>(
            &groupby),
        py::arg("data"), py::arg("group"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>());

  py::class_<GroupBy<T>> groupBy(m, name.c_str(), R"(
    GroupBy object implementing split-apply-combine mechanism.)");

  BIND_GROUPBY_OP(groupBy, mean);
  BIND_GROUPBY_OP(groupBy, sum);
  BIND_GROUPBY_OP(groupBy, nansum);
  BIND_GROUPBY_OP(groupBy, all);
  BIND_GROUPBY_OP(groupBy, any);
  BIND_GROUPBY_OP(groupBy, min);
  BIND_GROUPBY_OP(groupBy, nanmin);
  BIND_GROUPBY_OP(groupBy, max);
  BIND_GROUPBY_OP(groupBy, nanmax);
  BIND_GROUPBY_OP(groupBy, concat);
}

void init_groupby(py::module &m) {
  bind_groupby<DataArray>(m, "GroupByDataArray");
  bind_groupby<Dataset>(m, "GroupByDataset");
}
