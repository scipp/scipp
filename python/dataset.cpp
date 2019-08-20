// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <variant>

#include "scipp/core/dataset.h"
#include "scipp/core/except.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "pybind11.h"

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
}

template <class T, class... Ignored>
void bind_data_array_properties(py::class_<T, Ignored...> &c) {
  c.def_property_readonly("name", &T::name);
}

void init_dataset(py::module &m) {
  py::class_<Slice>(m, "Slice");

  bind_mutable_proxy<CoordsProxy, CoordsConstProxy>(m, "Coords");
  bind_mutable_proxy<LabelsProxy, LabelsConstProxy>(m, "Labels");
  bind_mutable_proxy<AttrsProxy, AttrsConstProxy>(m, "Attrs");

  py::class_<DataArray> dataArray(m, "DataArray");
  dataArray.def(py::init([](const std::optional<Variable> &data,
                            const std::map<Dim, Variable> &coords,
                            const std::map<std::string, Variable> &labels) {
                  Dataset d;
                  const std::string name = "";
                  if (data)
                    d.setData(name, *data);
                  for (const auto & [ dim, item ] : coords)
                    if (item.dims().sparse())
                      d.setSparseCoord(name, item);
                    else
                      d.setCoord(dim, item);
                  for (const auto & [ n, item ] : labels)
                    if (item.dims().sparse())
                      d.setSparseLabels(name, n, item);
                    else
                      d.setLabels(n, item);
                  return DataArray(d[name]);
                }),
                py::arg("data") = std::nullopt,
                py::arg("coords") = std::map<Dim, Variable>{},
                py::arg("labels") = std::map<std::string, Variable>{});
  py::class_<DataConstProxy>(m, "DataConstProxy");
  py::class_<DataProxy, DataConstProxy> dataProxy(m, "DataProxy");
  dataProxy.def_property(
      "data",
      py::cpp_function(
          [](const DataProxy &self) {
            return self.hasData() ? py::cast(self.data()) : py::none();
          },
          py::return_value_policy::move, py::keep_alive<0, 1>()),
      [](const DataProxy &self, const VariableConstProxy &data) {
        self.data().assign(data);
      });
  dataProxy.def("__repr__",
                [](const DataProxy &self) { return to_string(self); });

  bind_data_array_properties(dataArray);
  bind_data_array_properties(dataProxy);

  py::class_<DatasetConstProxy>(m, "DatasetConstProxy")
      .def(py::init<const Dataset &>());
  py::class_<DatasetProxy, DatasetConstProxy> datasetProxy(m, "DatasetProxy");
  datasetProxy.def(py::init<Dataset &>());

  py::class_<Dataset> dataset(m, "Dataset");
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
             auto[dim, i] = index;
             for (const auto[name, item] : self.slice(Slice(dim, i)))
               item.assign(other[name]);
           })
      .def("__delitem__", &Dataset::erase)
      .def("__setitem__",
           [](Dataset &self, const std::string &name, const DataArray &data) {
             self.setData(name, data);
           })
      .def("clear", &Dataset::clear);

  bind_dataset_proxy_methods(dataset);
  bind_dataset_proxy_methods(datasetProxy);

  bind_coord_properties(dataset);
  bind_coord_properties(datasetProxy);
  bind_coord_properties(dataArray);
  bind_coord_properties(dataProxy);

  bind_slice_methods(dataset);
  bind_slice_methods(datasetProxy);
  bind_slice_methods(dataProxy);

  bind_comparison<Dataset>(dataset);
  bind_comparison<DatasetProxy>(dataset);
  bind_comparison<Dataset>(datasetProxy);
  bind_comparison<DatasetProxy>(datasetProxy);
  bind_comparison<DataProxy>(dataProxy);

  bind_in_place_binary<Dataset>(dataset);
  bind_in_place_binary<DatasetProxy>(dataset);
  bind_in_place_binary<DataProxy>(dataset);
  bind_in_place_binary<Dataset>(datasetProxy);
  bind_in_place_binary<DatasetProxy>(datasetProxy);
  bind_in_place_binary<DataProxy>(datasetProxy);
  bind_in_place_binary<DataProxy>(dataProxy);

  bind_binary<Dataset>(dataset);
  bind_binary<DatasetProxy>(dataset);
  bind_binary<DataProxy>(dataset);
  bind_binary<Dataset>(datasetProxy);
  bind_binary<DatasetProxy>(datasetProxy);
  bind_binary<DataProxy>(datasetProxy);
  bind_binary<Dataset>(dataProxy);
  bind_binary<DatasetProxy>(dataProxy);
  bind_binary<DataProxy>(dataProxy);

  bind_data_properties(dataArray);
  bind_data_properties(dataProxy);

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
        "Returns a new Variabble with values in bins for sparse dims");

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

  m.def("concatenate",
        [](const DatasetConstProxy &lhs, const DatasetConstProxy &rhs,
           const Dim dim) { return core::concatenate(lhs, rhs, dim); },
        py::call_guard<py::gil_scoped_release>(),
        "Returns the concatenation of two datasets");

  py::implicitly_convertible<DataArray, DataConstProxy>();
  py::implicitly_convertible<Dataset, DatasetConstProxy>();
}
