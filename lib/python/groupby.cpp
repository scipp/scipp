// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
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

  groupBy.def(
      "mean",
      [](const GroupBy<T> &self, const std::string &dim) {
        return self.mean(Dim{dim});
      },
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_groupby<T>("mean").c_str());

  groupBy.def(
      "sum",
      [](const GroupBy<T> &self, const std::string &dim) {
        return self.sum(Dim{dim});
      },
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_groupby<T>("sum").c_str());

  groupBy.def(
      "all",
      [](const GroupBy<T> &self, const std::string &dim) {
        return self.all(Dim{dim});
      },
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_groupby<T>("all").c_str());

  groupBy.def(
      "any",
      [](const GroupBy<T> &self, const std::string &dim) {
        return self.any(Dim{dim});
      },
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_groupby<T>("any").c_str());

  groupBy.def(
      "min",
      [](const GroupBy<T> &self, const std::string &dim) {
        return self.min(Dim{dim});
      },
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_groupby<T>("min").c_str());

  groupBy.def(
      "max",
      [](const GroupBy<T> &self, const std::string &dim) {
        return self.max(Dim{dim});
      },
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_groupby<T>("max").c_str());

  groupBy.def(
      "concat",
      [](const GroupBy<T> &self, const std::string &dim) {
        return self.concat(Dim{dim});
      },
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      docstring_groupby<T>("concat").c_str());

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
