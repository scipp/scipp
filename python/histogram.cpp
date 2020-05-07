// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "detail.h"
#include "docstring.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;


// template <class T> void bind_flatten(py::module &m) {
//   m.def("flatten", py::overload_cast<CstViewRef<T>, const Dim>(&flatten),
//         py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
//         R"(
//         Flatten the specified dimension into event lists, equivalent to summing dense data.

//         :param x: Variable, DataArray, or Dataset to flatten.
//         :param dim: Dimension over which to flatten.
//         :raises: If the dimension does not exist, or if x does not contain event lists
//         :seealso: :py:class:`scipp.sum`
//         :return: New variable, data array, or dataset containing the flattened data.
//         :rtype: Variable, DataArray, or Dataset)");
// }




template <class T> void bind_histogram(py::module &m) {
  auto doc = Docstring()
        .returns("Histogrammed data with units of counts.")
        .rtype<T>();
  m.def(
      "histogram",
      [](CstViewRef<T> x, CstViewRef<Variable> bins) {
      // [](CstViewRef<T> x, const Variable &bins) {
        return dataset::histogram(x, bins);
      },
      py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
      doc.description("Histograms the input event data along the dimensions of "
                      "the supplied Variable describing the bin edges.")
         .param("x", "Input data to be histogrammed.")
         .param("bins", "Bin edges.").c_str());
  m.def(
      "histogram",
      [](CstViewRef<T> x) {
        return dataset::histogram(x);
      },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      doc.description("Accepts realigned data and histograms the unaligned "
                      "content according to the realigning axes.")
        .param("x", "Input realigned data to be histogrammed.").c_str());
}

//   m.def(
//       "histogram",
//       [](const DataArrayConstView &ds, const Variable &bins) {
//         return dataset::histogram(ds, bins);
//       },
//       py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
//       R"(Returns a new DataArray with values in bins for events dims.

//         :param x: Data to histogram.
//         :param bins: Bin edges.
//         :return: Histogramed data.
//         :rtype: DataArray)");

//   m.def(
//       "histogram",
//       [](const DataArrayConstView &ds, const VariableConstView &bins) {
//         return dataset::histogram(ds, bins);
//       },
//       py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
//       R"(Returns a new DataArray with values in bins for events dims.

//         :param x: Data to histogram.
//         :param bins: Bin edges.
//         :return: Histogramed data.
//         :rtype: DataArray)");

//   m.def(
//       "histogram",
//       [](const Dataset &ds, const VariableConstView &bins) {
//         return dataset::histogram(ds, bins);
//       },
//       py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
//       R"(Returns a new Dataset with values in bins for events dims.

//         :param x: Data to histogram.
//         :param bins: Bin edges.
//         :return: Histogramed data.
//         :rtype: Dataset)");

//   m.def(
//       "histogram",
//       [](const Dataset &ds, const Variable &bins) {
//         return dataset::histogram(ds, bins);
//       },
//       py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
//       R"(Returns a new Dataset with values in bins for events dims.

//         :param x: Data to histogram.
//         :param bins: Bin edges.
//         :return: Histogramed data.
//         :rtype: Dataset)");

// m.def(
//       "histogram",
//       [](const DataArrayConstView &x) { return dataset::histogram(x); },
//       py::arg("x"), py::call_guard<py::gil_scoped_release>(),
//       R"(Returns a new DataArray unaligned data content binned according to the realigning axes.

//         :param x: Realigned data to histogram.
//         :return: Histogramed data.
//         :rtype: DataArray)");
//   m.def(
//       "histogram",
//       [](const DatasetConstView &x) { return dataset::histogram(x); },
//       py::arg("x"), py::call_guard<py::gil_scoped_release>(),
//       R"(Returns a new Dataset unaligned data content binned according to the realigning axes.

//         :param x: Realigned data to histogram.
//         :return: Histogramed data.
//         :rtype: Dataset)");





void init_histogram(py::module &m) {
  // bind_histogram<DataArray>(m);
  // bind_histogram<Dataset>(m);
  auto doc = Docstring()
        .description("Histograms the input event data along the dimensions of "
                      "the supplied Variable describing the bin edges.")
         .returns("Histogrammed data with units of counts.")
         .param("x", "Input data to be histogrammed.")
         .param("bins", "Bin edges.");

  m.def(
      "histogram",
      [](CstViewRef<DataArray> x, CstViewRef<Variable> bins) {
        return histogram(x, bins);
      },
      py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
      doc
        .rtype("DataArray").c_str());

  m.def(
      "histogram",
      [](const Dataset &x, CstViewRef<Variable> bins) {
        return histogram(x, bins);
      },
      py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
      doc.rtype("Dataset").c_str());


  doc = Docstring()
        .description("Histograms the input event data along the dimensions of "
                      "the supplied Variable describing the bin edges.")
         .returns("Histogrammed data with units of counts.")
         .param("x", "Input data to be histogrammed.")
         .param("bins", "Bin edges.");

  m.def(
      "histogram",
      [](CstViewRef<DataArray> x) {
        return dataset::histogram(x);
      },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      doc.description("Accepts realigned data and histograms the unaligned "
                      "content according to the realigning axes.")
        .param("x", "Input realigned data to be histogrammed.").c_str());
}
