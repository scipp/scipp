// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/core/groupby.h"
#include "scipp/core/dataset.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

template <class T> void bind_groupby(py::module &m, const std::string &name) {
  py::class_<GroupBy<T>> groupBy(m, name.c_str(), R"(
    GroupBy object implementing to split-apply-combine mechanism.)");

  groupBy.def("mean", &GroupBy<T>::mean);
  groupBy.def("sum", &GroupBy<T>::sum);
}

void init_groupby(py::module &m) {
  bind_groupby<DataArray>(m, "GroupByDataArray");
  bind_groupby<Dataset>(m, "GroupByDataset");

  m.def("groupby",
        py::overload_cast<const DatasetConstProxy &, const std::string &,
                          const Dim>(&groupby),
        py::arg("data"), py::arg("labels"), py::arg("combine"),
        py::call_guard<py::gil_scoped_release>(),
        R"(Group dataset based on values of specified labels.

        :return: GroupBy helper object.
        :rtype: GroupBy)");
  m.def("groupby",
        py::overload_cast<const DatasetConstProxy &, const std::string &,
                          const VariableConstProxy &>(&groupby),
        py::arg("data"), py::arg("labels"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>(),
        R"(Group dataset based on values of specified labels.

        :return: GroupBy helper object.
        :rtype: GroupBy)");
}
