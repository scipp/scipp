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
      .def("__getitem__", &T::operator[])
      .def("__iter__",
           [](T &self) { return py::make_iterator(self.begin(), self.end()); });
  bind_comparison<T>(proxy);
}

template <class T, class... Ignored>
void bind_coord_properties(py::class_<T, Ignored...> &c) {
  c.def_property_readonly("coords", [](T &self) { return self.coords(); });
  c.def_property_readonly("labels", [](T &self) { return self.labels(); });
  c.def_property_readonly("attrs", [](T &self) { return self.attrs(); });
}

template <class T, class... Ignored>
void bind_dataset_proxy_methods(py::class_<T, Ignored...> &c) {
  c.def("__len__", &T::size);
  c.def("__repr__", [](const T &self) { return to_string(self); });
  c.def("__iter__",
        [](T &self) { return py::make_iterator(self.begin(), self.end()); });
  c.def("__getitem__",
        [](T &self, const std::string &name) { return self[name]; },
        py::keep_alive<0, 1>());
  c.def("__contains__", &T::contains);
  c.def("__eq__",
        [](const T &self, const Dataset &other) { return self == other; });
  c.def("__eq__",
        [](const T &self, const DatasetProxy &other) { return self == other; });
  c.def("__ne__",
        [](const T &self, const Dataset &other) { return self == other; });
  c.def("__ne__",
        [](const T &self, const DatasetProxy &other) { return self == other; });
}

void init_dataset(py::module &m) {
  py::class_<Slice>(m, "Slice");

  bind_mutable_proxy<CoordsProxy, CoordsConstProxy>(m, "Coords");
  bind_mutable_proxy<LabelsProxy, LabelsConstProxy>(m, "Labels");
  bind_mutable_proxy<AttrsProxy, AttrsConstProxy>(m, "Attrs");

  py::class_<DataConstProxy>(m, "DataConstProxy");
  py::class_<DataProxy, DataConstProxy> dataProxy(m, "DataProxy");
  dataProxy.def_property_readonly("data", &DataProxy::data,
                                  py::keep_alive<0, 1>());
  dataProxy.def("__repr__",
                [](const DataProxy &self) { return to_string(self); });

  py::class_<DatasetConstProxy>(m, "DatasetConstProxy");
  py::class_<DatasetProxy, DatasetConstProxy> datasetProxy(m, "DatasetProxy");

  py::class_<Dataset> dataset(m, "Dataset");
  dataset.def(py::init<>())
      .def(py::init([](const std::map<std::string, Variable> &data,
                       const std::map<Dim, Variable> &coords,
                       const std::map<std::string, Variable> &labels) {
             Dataset d;
             for (const auto & [ name, item ] : data)
               d.setData(name, item);
             for (const auto & [ dim, item ] : coords)
               d.setCoord(dim, item);
             for (const auto & [ name, item ] : labels)
               d.setLabels(name, item);
             return d;
           }),
           py::arg("data") = std::map<std::string, Variable>{},
           py::arg("coords") = std::map<Dim, Variable>{},
           py::arg("labels") = std::map<std::string, Variable>{})
      .def("__setitem__", [](Dataset &self, const std::string &name,
                             Variable data) { self.setData(name, data); })
      .def("__setitem__",
           [](Dataset &self, const std::string &name,
              const DataConstProxy &data) { self.setData(name, data); })
      .def("__setitem__",
           [](Dataset &self, const std::string &name, const DataProxy &data) {
             if (self.contains(name))
               self[name].assign(data);
             else
               throw std::runtime_error("Not implemented yet");
           })
      .def("set_sparse_coord", &Dataset::setSparseCoord)
      .def("set_sparse_labels", &Dataset::setSparseLabels)
      .def("set_coord", &Dataset::setCoord)
      .def("set_labels", &Dataset::setLabels)
      .def("set_attr", &Dataset::setAttr);

  bind_dataset_proxy_methods(dataset);
  bind_dataset_proxy_methods(datasetProxy);

  bind_coord_properties(dataset);
  bind_coord_properties(datasetProxy);
  bind_coord_properties(dataProxy);

  bind_slice_methods(dataset);
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
  // Implicit conversion DatasetProxy -> Dataset. Reduces need for
  // excessive operator overload definitions
  py::implicitly_convertible<DatasetProxy, Dataset>();
}
