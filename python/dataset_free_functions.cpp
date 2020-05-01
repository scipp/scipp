// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"
#include "scipp/dataset/histogram.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

void init_dataset_free_functions(py::module &m) {

  m.def("histogram",
        [](const DataArrayConstView &ds, const Variable &bins) {
          return dataset::histogram(ds, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        R"(Returns a new DataArray with values in bins for events dims.

        :param x: Data to histogram.
        :param bins: Bin edges.
        :return: Histogramed data.
        :rtype: DataArray)");

  m.def("histogram",
        [](const DataArrayConstView &ds, const VariableConstView &bins) {
          return dataset::histogram(ds, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        R"(Returns a new DataArray with values in bins for events dims.

        :param x: Data to histogram.
        :param bins: Bin edges.
        :return: Histogramed data.
        :rtype: DataArray)");

  m.def("histogram",
        [](const Dataset &ds, const VariableConstView &bins) {
          return dataset::histogram(ds, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        R"(Returns a new Dataset with values in bins for events dims.

        :param x: Data to histogram.
        :param bins: Bin edges.
        :return: Histogramed data.
        :rtype: Dataset)");

  m.def("histogram",
        [](const Dataset &ds, const Variable &bins) {
          return dataset::histogram(ds, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        R"(Returns a new Dataset with values in bins for events dims.

        :param x: Data to histogram.
        :param bins: Bin edges.
        :return: Histogramed data.
        :rtype: Dataset)");

  m.def(
      "histogram",
      [](const DataArrayConstView &x) { return dataset::histogram(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      R"(Returns a new DataArray unaligned data content binned according to the realigning axes.

        :param x: Realigned data to histogram.
        :return: Histogramed data.
        :rtype: DataArray)");

  m.def(
      "histogram",
      [](const DatasetConstView &x) { return dataset::histogram(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      R"(Returns a new Dataset unaligned data content binned according to the realigning axes.

        :param x: Realigned data to histogram.
        :return: Histogramed data.
        :rtype: Dataset)");

  m.def("rebin",
        py::overload_cast<const DataArrayConstView &, const Dim,
                          const VariableConstView &>(&rebin),
        py::arg("x"), py::arg("dim"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>(), R"(
        Rebin a dimension of a data array.

        :param x: Data to rebin.
        :param dim: Dimension to rebin over.
        :param bins: New bin edges.
        :raises: If data cannot be rebinned, e.g., if the unit is not counts, or the existing coordinate is not a bin-edge coordinate.
        :return: A new data array with data rebinned according to the new coordinate.
        :rtype: DataArray)");

  m.def("rebin",
        py::overload_cast<const DatasetConstView &, const Dim,
                          const VariableConstView &>(&rebin),
        py::arg("x"), py::arg("dim"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>(), R"(
        Rebin a dimension of a dataset.

        :param x: Data to rebin.
        :param dim: Dimension to rebin over.
        :param bins: New bin edges.
        :raises: If data cannot be rebinned, e.g., if the unit is not counts, or the existing coordinate is not a bin-edge coordinate.
        :return: A new dataset with data rebinned according to the new coordinate.
        :rtype: Dataset)");

  m.def("map", event::map, py::arg("function"), py::arg("iterable"),
        py::arg("dim") = to_string(Dim::Invalid),
        py::call_guard<py::gil_scoped_release>(),
        R"(Return mapped event data.

        This only supports event data.

        :param function: Data array serving as a discretized mapping function.
        :param iterable: Variable with values to map, must be event data.
        :param dim: Optional dimension to use for mapping, if not given, `map` will attempt to determine the dimension from the `function` argument.
        :return: Mapped event data.
        :rtype: Variable)");

  m.def("merge",
        [](const DatasetConstView &lhs, const DatasetConstView &rhs) {
          return dataset::merge(lhs, rhs);
        },
        py::arg("lhs"), py::arg("rhs"),
        py::call_guard<py::gil_scoped_release>(), R"(
        Union of two datasets.

        :param lhs: First Dataset.
        :param rhs: Second Dataset.
        :raises: If there are conflicting items with different content.
        :return: A new dataset that contains the union of all data items, coords, masks and attributes.
        :rtype: Dataset)");

  // m.def("realign",
  //       [](const DataArrayConstView &a, py::dict coord_dict) {
  //         DataArray copy(a);
  //         realign_impl(copy, coord_dict);
  //         return copy;
  //       },
  //       py::arg("data"), py::arg("coords"));
  // m.def("realign",
  //       [](const DatasetConstView &a, py::dict coord_dict) {
  //         Dataset copy(a);
  //         realign_impl(copy, coord_dict);
  //         return copy;
  //       },
  //       py::arg("data"), py::arg("coords"));

  m.def("combine_masks",
        [](const MasksConstView &msk, const std::vector<Dim> &labels,
           const std::vector<scipp::index> &shape) {
          return dataset::masks_merge_if_contained(msk,
                                                   Dimensions(labels, shape));
        },
        py::call_guard<py::gil_scoped_release>(), R"(
        Combine all masks into a single one following the OR operation.
        This requires a masks view as an input, followed by the dimension
        labels and shape of the Variable/DataArray. The labels and the shape
        are used to create a Dimensions object. The function then iterates
        through the masks view and combines only the masks that have all
        their dimensions contained in the Variable/DataArray Dimensions.

        :return: A new variable that contains the union of all masks.
        :rtype: Variable)");
}
