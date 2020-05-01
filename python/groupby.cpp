// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/groupby.h"
#include "scipp/dataset/dataset.h"

#include "pybind11.h"
#include "bind_free_function.h"

using namespace scipp;
using namespace scipp::dataset;
using namespace scipp::python;

namespace py = pybind11;

template <class T> void bind_groupby(py::module &m, const std::string &name) {

  using VAConstView = const typename Variable::const_view_type &;
  using ConstView = const typename T::const_view_type &;
  Docstring docs;

  // Flatten
  docs = {
    // Description
    "Group dataset or data array based on values of specified labels.",
    // Raises
    "",
    // See also
    "",
    // Returns
    "GroupBy helper object.",
    // Return type
    "GroupByDataArray or GroupByDataset.",
    // Input parameters
    {{"data", "Input Dataset or DataArray."},
    {"group", "String containing name of labels to use for grouping."}}
  };
  // Overload on function that accepts docstring is ambiguous, use long version of constructor
  bind_free_function<GroupBy<T>, ConstView, const Dim>(groupby, "groupby", m, docs.param(0),
    docs.param(1), docs.description(), docs.raises(), docs.seealso(), docs.returns(), docs.rtype());
  docs.insert_param(2, {"bins", "Bins for grouping label values."});
  bind_free_function<GroupBy<T>, ConstView, const Dim, VAConstView>(groupby, "groupby", m, docs.param(0),
    docs.param(1), docs.param(2), docs.description(), docs.raises(), docs.seealso(), docs.returns(), docs.rtype());
  


  py::class_<GroupBy<T>> groupBy(m, name.c_str(), R"(
    GroupBy object implementing to split-apply-combine mechanism.)");

    bind_free_function<T, const Dim>(&GroupBy<T>::flatten, "flatten", groupBy, docs);

  // groupBy.def("flatten", &GroupBy<T>::flatten, py::arg("dim"),
  //             py::call_guard<py::gil_scoped_release>(), R"(
  //     Flatten specified dimension into event lists.

  //     This is a event-data equivalent to calling ``sum`` on dense data.
  //     In particular, summing the result of histogrammed data yields the same result as histgramming data that has been flattened.

  //     :param dim: Dimension to flatten
  //     :type dim: Dim
  //     :return: Flattened event data for each group, combined along dimension specified when calling :py:func:`scipp.groupby`
  //     :rtype: DataArray or Dataset)");

  groupBy.def("mean", &GroupBy<T>::mean, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise mean over the specified dimension within a group.

      :param dim: Dimension to sum over when computing the mean
      :type dim: Dim
      :return: Mean over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("sum", &GroupBy<T>::sum, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise sum over the specified dimension within a group.

      :param dim: Dimension to sum over
      :type dim: Dim
      :return: Sum over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("all", &GroupBy<T>::all, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise AND over the specified dimension within a group.

      :param dim: Dimension to reduce
      :type dim: Dim
      :return: AND over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("any", &GroupBy<T>::any, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise OR over the specified dimension within a group.

      :param dim: Dimension to reduce
      :type dim: Dim
      :return: OR over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("max", &GroupBy<T>::max, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise max over the specified dimension within a group.

      :param dim: Dimension to reduce
      :type dim: Dim
      :return: Max over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("min", &GroupBy<T>::min, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise min over the specified dimension within a group.

      :param dim: Dimension to reduce
      :type dim: Dim
      :return: Min over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");
}

void init_groupby(py::module &m) {
  bind_groupby<DataArray>(m, "GroupByDataArray");
  bind_groupby<Dataset>(m, "GroupByDataset");
}
