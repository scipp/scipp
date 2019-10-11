// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/sort.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "pybind11.h"
#include "rename.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

template <class T, class ConstT>
void bind_mutable_proxy(py::module &m, const std::string &name) {
  py::class_<ConstT>(m, (name + "ConstProxy").c_str());
  py::class_<T, ConstT> proxy(m, (name + "Proxy").c_str());
  proxy.def("__len__", &T::size)
      .def("__getitem__", &T::operator[], py::return_value_policy::move,
           py::keep_alive<0, 1>())
      .def("__setitem__", &T::set)
      .def("__delitem__", &T::erase)
      .def("__iter__",
           [](T &self) {
             return py::make_iterator(self.begin(), self.end(),
                                      py::return_value_policy::move);
           },
           py::keep_alive<0, 1>())
      .def("__contains__", &T::contains);
  bind_comparison<T>(proxy);
}

template <class T, class... Ignored>
void bind_coord_properties(py::class_<T, Ignored...> &c) {
  // For some reason the return value policy and/or keep-alive policy do not
  // work unless we wrap things in py::cpp_function.
  c.def_property_readonly(
      "coords",
      py::cpp_function([](T &self) { return self.coords(); },
                       py::return_value_policy::move, py::keep_alive<0, 1>()),
      R"(
      Dict of coordinates.)");
  c.def_property_readonly(
      "labels",
      py::cpp_function([](T &self) { return self.labels(); },
                       py::return_value_policy::move, py::keep_alive<0, 1>()),
      R"(
      Dict of labels.

      Labels are very similar to coordinates, except that they are identified
      using custom names instead of dimension labels.)");
  c.def_property_readonly("attrs",
                          py::cpp_function([](T &self) { return self.attrs(); },
                                           py::return_value_policy::move,
                                           py::keep_alive<0, 1>()),
                          R"(
      Dict of attributes.)");
}

template <class T, class... Ignored>
void bind_dataset_proxy_methods(py::class_<T, Ignored...> &c) {
  c.def("__len__", &T::size);
  c.def("__repr__", [](const T &self) { return to_string(self); });
  c.def("__iter__",
        [](T &self) {
          return py::make_iterator(self.begin(), self.end(),
                                   py::return_value_policy::move);
        },
        py::return_value_policy::move, py::keep_alive<0, 1>());
  c.def("__getitem__",
        [](T &self, const std::string &name) { return self[name]; },
        py::keep_alive<0, 1>());
  c.def("__contains__", &T::contains);
  c.def("copy", [](const T &self) { return Dataset(self); },
        "Return a (deep) copy.");
  c.def_property_readonly("dims",
                          [](const T &self) {
                            std::vector<Dim> dims;
                            for (const auto &dim : self.dimensions()) {
                              dims.push_back(dim.first);
                            }
                            return dims;
                          },
                          R"(List of dimensions.)",
                          py::return_value_policy::move);
  c.def_property_readonly("shape",
                          [](const T &self) {
                            std::vector<scipp::index> shape;
                            for (const auto &dim : self.dimensions()) {
                              shape.push_back(dim.second);
                            }
                            return shape;
                          },
                          R"(List of shapes.)", py::return_value_policy::move);
}

template <class T, class... Ignored>
void bind_data_array_properties(py::class_<T, Ignored...> &c) {
  c.def_property_readonly("name", &T::name, R"(The name of the held data.)");
  c.def("__repr__", [](const T &self) { return to_string(self); });
  c.def("copy", [](const T &self) { return DataArray(self); },
        "Return a (deep) copy.");
  c.def_property(
      "data",
      py::cpp_function(
          [](T &self) {
            return self.hasData() ? py::cast(self.data()) : py::none();
          },
          py::return_value_policy::move, py::keep_alive<0, 1>()),
      [](T &self, const VariableConstProxy &data) { self.data().assign(data); },
      R"(Underlying data item.)");
  bind_coord_properties(c);
  bind_comparison<DataConstProxy>(c);
  bind_data_properties(c);
  bind_slice_methods(c);
  bind_in_place_binary<DataProxy>(c);
  bind_in_place_binary<VariableConstProxy>(c);
  bind_binary<Dataset>(c);
  bind_binary<DatasetProxy>(c);
  bind_binary<DataProxy>(c);
  bind_binary<VariableConstProxy>(c);
}

void init_dataset(py::module &m) {
  py::class_<Slice>(m, "Slice");

  bind_mutable_proxy<CoordsProxy, CoordsConstProxy>(m, "Coords");
  bind_mutable_proxy<LabelsProxy, LabelsConstProxy>(m, "Labels");
  bind_mutable_proxy<AttrsProxy, AttrsConstProxy>(m, "Attrs");

  py::class_<DataArray> dataArray(m, "DataArray", R"(
    Named variable with associated coords, labels, and attributes.)");
  dataArray.def(py::init<const DataConstProxy &>());
  dataArray.def(
      py::init<const std::optional<Variable> &, const std::map<Dim, Variable> &,
               const std::map<std::string, Variable> &>(),
      py::arg("data") = std::nullopt,
      py::arg("coords") = std::map<Dim, Variable>{},
      py::arg("labels") = std::map<std::string, Variable>{});

  py::class_<DataConstProxy>(m, "DataConstProxy")
      .def(py::init<const DataArray &>());

  py::class_<DataProxy, DataConstProxy> dataProxy(m, "DataProxy", R"(
        Proxy for DataArray, representing a sliced view onto a DataArray, or an item of a Dataset;
        Mostly equivalent to DataArray, see there for details.)");
  dataProxy.def(py::init<DataArray &>());

  bind_data_array_properties(dataArray);
  bind_data_array_properties(dataProxy);

  py::class_<DatasetConstProxy>(m, "DatasetConstProxy")
      .def(py::init<const Dataset &>());
  py::class_<DatasetProxy, DatasetConstProxy> datasetProxy(m, "DatasetProxy",
                                                           R"(
        Proxy for Dataset, representing a sliced view onto a Dataset;
        Mostly equivalent to Dataset, see there for details.)");
  datasetProxy.def(py::init<Dataset &>());

  py::class_<Dataset> dataset(m, "Dataset", R"(
    Dict of data arrays with aligned dimensions.)");

  dataset.def(py::init<const std::map<std::string, DataConstProxy> &>())
      .def(py::init<const DataConstProxy &>())
      .def(py::init([](const std::map<std::string, VariableConstProxy> &data,
                       const std::map<Dim, VariableConstProxy> &coords,
                       const std::map<std::string, VariableConstProxy> &labels,
                       const std::map<std::string, VariableConstProxy> &attrs) {
             return Dataset(data, coords, labels, attrs);
           }),
           py::arg("data") = std::map<std::string, VariableConstProxy>{},
           py::arg("coords") = std::map<Dim, VariableConstProxy>{},
           py::arg("labels") = std::map<std::string, VariableConstProxy>{},
           py::arg("attrs") = std::map<std::string, VariableConstProxy>{})
      .def(py::init([](const DatasetProxy &other) { return Dataset{other}; }))
      .def("__setitem__",
           [](Dataset &self, const std::string &name,
              const VariableConstProxy &data) { self.setData(name, data); })
      .def("__setitem__",
           [](Dataset &self, const std::string &name,
              const DataConstProxy &data) { self.setData(name, data); })
      .def("__setitem__",
           [](Dataset &self, const std::tuple<Dim, scipp::index> &index,
              DatasetProxy &other) {
             auto [dim, i] = index;
             for (const auto [name, item] : self.slice(Slice(dim, i)))
               item.assign(other[name]);
           })
      .def("__delitem__", &Dataset::erase)
      .def("__setitem__",
           [](Dataset &self, const std::string &name, const DataArray &data) {
             self.setData(name, data);
           })
      .def(
          "clear", &Dataset::clear,
          R"(Removes all data (preserving coordinates, attributes and labels).)");

  bind_dataset_proxy_methods(dataset);
  bind_dataset_proxy_methods(datasetProxy);

  bind_coord_properties(dataset);
  bind_coord_properties(datasetProxy);

  bind_slice_methods(dataset);
  bind_slice_methods(datasetProxy);

  bind_comparison<Dataset>(dataset);
  bind_comparison<DatasetProxy>(dataset);
  bind_comparison<Dataset>(datasetProxy);
  bind_comparison<DatasetProxy>(datasetProxy);

  bind_in_place_binary<Dataset>(dataset);
  bind_in_place_binary<DatasetProxy>(dataset);
  bind_in_place_binary<DataProxy>(dataset);
  bind_in_place_binary<VariableConstProxy>(dataset);
  bind_in_place_binary<Dataset>(datasetProxy);
  bind_in_place_binary<DatasetProxy>(datasetProxy);
  bind_in_place_binary<VariableConstProxy>(datasetProxy);
  bind_in_place_binary<DataProxy>(datasetProxy);
  bind_in_place_binary_scalars(dataset);
  bind_in_place_binary_scalars(datasetProxy);
  bind_in_place_binary_scalars(dataArray);
  bind_in_place_binary_scalars(dataProxy);

  bind_binary<Dataset>(dataset);
  bind_binary<DatasetProxy>(dataset);
  bind_binary<DataProxy>(dataset);
  bind_binary<VariableConstProxy>(dataset);
  bind_binary<Dataset>(datasetProxy);
  bind_binary<DatasetProxy>(datasetProxy);
  bind_binary<DataProxy>(datasetProxy);
  bind_binary<VariableConstProxy>(datasetProxy);

  dataArray.def("rename_dims", &rename_dims<DataArray>, py::arg("dims_dict"),
                "Rename dimensions.");
  dataset.def("rename_dims", &rename_dims<Dataset>, py::arg("dims_dict"),
              "Rename dimensions.");

  m.def("concatenate",
        py::overload_cast<const DataConstProxy &, const DataConstProxy &,
                          const Dim>(&concatenate),
        py::call_guard<py::gil_scoped_release>(), R"(
        Concatenate input data array along the given dimension.

        Concatenates the data, coords, and labels of the data array.
        Coords and labels for any but the given dimension are required to match and are copied to the output without changes.

        :raises: If the dtype or unit does not match, or if the dimensions and shapes are incompatible.
        :return: New data array containing all data, coords, and labels of the input arrays.
        :rtype: DataArray)");

  m.def("concatenate",
        py::overload_cast<const DatasetConstProxy &, const DatasetConstProxy &,
                          const Dim>(&concatenate),
        py::call_guard<py::gil_scoped_release>(), R"(
        Concatenate input datasets along the given dimension.

        Concatenate all cooresponding items in the input datasets.
        The output contains only items that are present in both inputs.

        :raises: If the dtype or unit does not match, or if the dimensions and shapes are incompatible.
        :return: New dataset.
        :rtype: Dataset)");

  m.def("histogram",
        [](const DataConstProxy &ds, const Variable &bins) {
          return core::histogram(ds, bins);
        },
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable with values in bins for for sparse dims");

  m.def("histogram",
        [](const DataConstProxy &ds, const VariableConstProxy &bins) {
          return core::histogram(ds, bins);
        },
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable with values in bins for sparse dims");

  m.def("histogram",
        [](const Dataset &ds, const VariableConstProxy &bins) {
          return core::histogram(ds, bins);
        },
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Dataset with histograms for sparse dims");

  m.def("histogram",
        [](const Dataset &ds, const Variable &bins) {
          return core::histogram(ds, bins);
        },
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Dataset with histograms for sparse dims");

  m.def("merge",
        [](const DatasetConstProxy &lhs, const DatasetConstProxy &rhs) {
          return core::merge(lhs, rhs);
        },
        py::call_guard<py::gil_scoped_release>(), R"(
        Union of two datasets.

        :raises: If there are conflicting items with different content.
        :return: A new dataset that contains the union of all data items, coords, labels, and attributes.
        :rtype: Dataset)");

  m.def("sum", py::overload_cast<const DataConstProxy &, const Dim>(&sum),
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise sum over the specified dimension.

        :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New data array containing the sum.
        :rtype: DataArray)");

  m.def("sum", py::overload_cast<const DatasetConstProxy &, const Dim>(&sum),
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise sum over the specified dimension.

        :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New dataset containing the sum for each data item.
        :rtype: Dataset)");

  m.def("mean", py::overload_cast<const DataConstProxy &, const Dim>(&mean),
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise mean over the specified dimension, if variances are present, the new variance is computated as standard-deviation of the mean.

        See the documentation for the mean of a Variable for details in the computation of the ouput variance.

        :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New data array containing the mean for each data item.
        :rtype: DataArray)");

  m.def("mean", py::overload_cast<const DatasetConstProxy &, const Dim>(&mean),
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise mean over the specified dimension, if variances are present, the new variance is computated as standard-deviation of the mean.

        See the documentation for the mean of a Variable for details in the computation of the ouput variance.

        :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New dataset containing the mean for each data item.
        :rtype: Dataset)");

  m.def("rebin",
        py::overload_cast<const DataConstProxy &, const Dim,
                          const VariableConstProxy &>(&rebin),
        py::call_guard<py::gil_scoped_release>(), R"(
        Rebin a dimension of a data array.

        :raises: If data cannot be rebinned, e.g., if the unit is not counts, or the existing coordinate is not a bin-edge coordinate.
        :return: A new data array with data rebinned according to the new coordinate.
        :rtype: DataArray)");
  m.def("rebin",
        py::overload_cast<const DatasetConstProxy &, const Dim,
                          const VariableConstProxy &>(&rebin),
        py::call_guard<py::gil_scoped_release>(), R"(
        Rebin a dimension of a dataset.

        :raises: If data cannot be rebinned, e.g., if the unit is not counts, or the existing coordinate is not a bin-edge coordinate.
        :return: A new dataset with data rebinned according to the new coordinate.
        :rtype: Dataset)");

  m.def("sort",
        py::overload_cast<const DataConstProxy &, const VariableConstProxy &>(
            &sort),
        py::arg("data"), py::arg("key"),
        py::call_guard<py::gil_scoped_release>(),
        R"(Sort data array along a dimension by a sort key.

        :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
        :return: New sorted data array.
        :rtype: DataArray)");
  m.def(
      "sort", py::overload_cast<const DataConstProxy &, const Dim &>(&sort),
      py::arg("data"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      R"(Sort data array along a dimension by the coordinate values for that dimension.

      :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
      :return: New sorted data array.
      :rtype: DataArray)");
  m.def(
      "sort",
      py::overload_cast<const DataConstProxy &, const std::string &>(&sort),
      py::arg("data"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      R"(Sort data array along a dimension by the label values for the given key.

      :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
      :return: New sorted data array.
      :rtype: DataArray)");

  m.def(
      "sort",
      py::overload_cast<const DatasetConstProxy &, const VariableConstProxy &>(
          &sort),
      py::arg("data"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      R"(Sort dataset along a dimension by a sort key.

        :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
        :return: New sorted dataset.
        :rtype: Dataset)");
  m.def(
      "sort", py::overload_cast<const DatasetConstProxy &, const Dim &>(&sort),
      py::arg("data"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      R"(Sort dataset along a dimension by the coordinate values for that dimension.

      :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
      :return: New sorted dataset.
      :rtype: Dataset)");
  m.def(
      "sort",
      py::overload_cast<const DatasetConstProxy &, const std::string &>(&sort),
      py::arg("data"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      R"(Sort dataset along a dimension by the label values for the given key.

      :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
      :return: New sorted dataset.
      :rtype: Dataset)");

  py::implicitly_convertible<DataArray, DataConstProxy>();
  py::implicitly_convertible<DataArray, DataProxy>();
  py::implicitly_convertible<Dataset, DatasetConstProxy>();
}
