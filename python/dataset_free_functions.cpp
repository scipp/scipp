// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"
// #include "scipp/dataset/except.h"
#include "scipp/dataset/histogram.h"
// #include "scipp/dataset/map_view.h"
// #include "scipp/dataset/sort.h"
// #include "scipp/dataset/unaligned.h"

// #include "bind_data_access.h"
// #include "bind_operators.h"
// #include "bind_slice_methods.h"
// #include "detail.h"
#include "pybind11.h"
// #include "rename.h"
// #include "views.h"

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

  // m.def("sum", py::overload_cast<const DataArrayConstView &, const Dim>(&sum),
  //       py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
  //       R"(
  //       Element-wise sum over the specified dimension.

  //       :param x: Data to sum.
  //       :param dim: Dimension over which to sum.
  //       :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
  //       :seealso: :py:class:`scipp.mean`
  //       :return: New data array containing the sum.
  //       :rtype: DataArray)");

  // m.def("sum", py::overload_cast<const DatasetConstView &, const Dim>(&sum),
  //       py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
  //       R"(
  //       Element-wise sum over the specified dimension.

  //       :param x: Data to sum.
  //       :param dim: Dimension over which to sum.
  //       :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
  //       :seealso: :py:class:`scipp.mean`
  //       :return: New dataset containing the sum for each data item.
  //       :rtype: Dataset)");

  // m.def("mean", py::overload_cast<const DataArrayConstView &, const Dim>(&mean),
  //       py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
  //       R"(
  //       Element-wise mean over the specified dimension, if variances are present, the new variance is computated as standard-deviation of the mean.

  //       See the documentation for the mean of a Variable for details in the computation of the ouput variance.

  //       :param x: Data to calculate mean of.
  //       :param dim: Dimension over which to calculate mean.
  //       :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
  //       :seealso: :py:class:`scipp.mean`
  //       :return: New data array containing the mean for each data item.
  //       :rtype: DataArray)");

  // m.def("mean", py::overload_cast<const DatasetConstView &, const Dim>(&mean),
  //       py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
  //       R"(
  //       Element-wise mean over the specified dimension, if variances are present, the new variance is computated as standard-deviation of the mean.

  //       See the documentation for the mean of a Variable for details in the computation of the ouput variance.

  //       :param x: Data to calculate mean of.
  //       :param dim: Dimension over which to calculate mean.
  //       :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
  //       :seealso: :py:class:`scipp.mean`
  //       :return: New dataset containing the mean for each data item.
  //       :rtype: Dataset)");

  // m.def("rebin",
  //       py::overload_cast<const DataArrayConstView &, const Dim,
  //                         const VariableConstView &>(&rebin),
  //       py::arg("x"), py::arg("dim"), py::arg("bins"),
  //       py::call_guard<py::gil_scoped_release>(), R"(
  //       Rebin a dimension of a data array.

  //       :param x: Data to rebin.
  //       :param dim: Dimension to rebin over.
  //       :param bins: New bin edges.
  //       :raises: If data cannot be rebinned, e.g., if the unit is not counts, or the existing coordinate is not a bin-edge coordinate.
  //       :return: A new data array with data rebinned according to the new coordinate.
  //       :rtype: DataArray)");

  // m.def("rebin",
  //       py::overload_cast<const DatasetConstView &, const Dim,
  //                         const VariableConstView &>(&rebin),
  //       py::arg("x"), py::arg("dim"), py::arg("bins"),
  //       py::call_guard<py::gil_scoped_release>(), R"(
  //       Rebin a dimension of a dataset.

  //       :param x: Data to rebin.
  //       :param dim: Dimension to rebin over.
  //       :param bins: New bin edges.
  //       :raises: If data cannot be rebinned, e.g., if the unit is not counts, or the existing coordinate is not a bin-edge coordinate.
  //       :return: A new dataset with data rebinned according to the new coordinate.
  //       :rtype: Dataset)");

  // m.def(
  //     "sort",
  //     py::overload_cast<const DataArrayConstView &, const VariableConstView &>(
  //         &sort),
  //     py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
  //     R"(Sort data array along a dimension by a sort key.

  //       :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
  //       :return: New sorted data array.
  //       :rtype: DataArray)");

  // m.def(
  //     "sort", py::overload_cast<const DataArrayConstView &, const Dim &>(&sort),
  //     py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
  //     R"(Sort data array along a dimension by the coordinate values for that dimension.

  //     :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
  //     :return: New sorted data array.
  //     :rtype: DataArray)");

  // m.def("sort",
  //       py::overload_cast<const DatasetConstView &, const VariableConstView &>(
  //           &sort),
  //       py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
  //       R"(Sort dataset along a dimension by a sort key.

  //       :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
  //       :return: New sorted dataset.
  //       :rtype: Dataset)");

  // m.def(
  //     "sort", py::overload_cast<const DatasetConstView &, const Dim &>(&sort),
  //     py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
  //     R"(Sort dataset along a dimension by the coordinate values for that dimension.

  //     :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
  //     :return: New sorted dataset.
  //     :rtype: Dataset)");

  // m.def("combine_masks",
  //       [](const MasksConstView &msk, const std::vector<Dim> &labels,
  //          const std::vector<scipp::index> &shape) {
  //         return dataset::masks_merge_if_contained(msk,
  //                                                  Dimensions(labels, shape));
  //       },
  //       py::call_guard<py::gil_scoped_release>(), R"(
  //       Combine all masks into a single one following the OR operation.
  //       This requires a masks view as an input, followed by the dimension
  //       labels and shape of the Variable/DataArray. The labels and the shape
  //       are used to create a Dimensions object. The function then iterates
  //       through the masks view and combines only the masks that have all
  //       their dimensions contained in the Variable/DataArray Dimensions.

  //       :return: A new variable that contains the union of all masks.
  //       :rtype: Variable)");

  // m.def("reciprocal",
  //       [](const DataArrayConstView &self) { return reciprocal(self); },
  //       py::arg("x"), py::call_guard<py::gil_scoped_release>(), R"(
  //       Element-wise reciprocal.

  //       :return: Reciprocal of the input values.
  //       :rtype: DataArray)");

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
  // m.def("filter", filter_impl<DataArray>, py::arg("data"), py::arg("filter"),
  //       py::arg("interval"), py::arg("keep_attrs") = true,
  //       py::call_guard<py::gil_scoped_release>(),
  //       R"(Return filtered event data.

  //       This only supports event data.

  //       :param data: Input event data
  //       :param filter: Name of coord to use for filtering
  //       :param interval: Variable defining the valid interval of coord values to include in the output
  //       :param keep_attrs: If False, attributes are not copied to the output, default is True
  //       :return: Filtered data.
  //       :rtype: DataArray)");
  // m.def(
  //     "histogram",
  //     [](const DataArrayConstView &x) { return dataset::histogram(x); },
  //     py::arg("x"), py::call_guard<py::gil_scoped_release>(),
  //     R"(Returns a new DataArray unaligned data content binned according to the realigning axes.

  //       :param x: Realigned data to histogram.
  //       :return: Histogramed data.
  //       :rtype: DataArray)");
  // m.def(
  //     "histogram",
  //     [](const DatasetConstView &x) { return dataset::histogram(x); },
  //     py::arg("x"), py::call_guard<py::gil_scoped_release>(),
  //     R"(Returns a new Dataset unaligned data content binned according to the realigning axes.

  //       :param x: Realigned data to histogram.
  //       :return: Histogramed data.
  //       :rtype: Dataset)");
  // m.def("map", event::map, py::arg("function"), py::arg("iterable"),
  //       py::arg("dim") = to_string(Dim::Invalid),
  //       py::call_guard<py::gil_scoped_release>(),
  //       R"(Return mapped event data.

  //       This only supports event data.

  //       :param function: Data array serving as a discretized mapping function.
  //       :param iterable: Variable with values to map, must be event data.
  //       :param dim: Optional dimension to use for mapping, if not given, `map` will attempt to determine the dimension from the `function` argument.
  //       :return: Mapped event data.
  //       :rtype: Variable)");

  // bind_astype(dataArray);
  // bind_astype(dataArrayView);

  // py::implicitly_convertible<DataArray, DataArrayConstView>();
  // py::implicitly_convertible<DataArray, DataArrayView>();
  // py::implicitly_convertible<Dataset, DatasetConstView>();
}
