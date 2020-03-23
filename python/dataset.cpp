// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/histogram.h"
#include "scipp/core/sort.h"
#include "scipp/core/unaligned.h"
#include "scipp/core/view_decl.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "detail.h"
#include "pybind11.h"
#include "rename.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

/// Helper to provide equivalent of the `items()` method of a Python dict.
template <class T> class items_view {
public:
  items_view(T &obj) : m_obj(&obj) {}
  auto size() const noexcept { return m_obj->size(); }
  auto begin() const { return m_obj->items_begin(); }
  auto end() const { return m_obj->items_end(); }

private:
  T *m_obj;
};
template <class T> items_view(T &)->items_view<T>;

/// Helper to provide equivalent of the `values()` method of a Python dict.
template <class T> class values_view {
public:
  values_view(T &obj) : m_obj(&obj) {}
  auto size() const noexcept { return m_obj->size(); }
  auto begin() const {
    if constexpr (std::is_same_v<typename T::mapped_type, DataArray>)
      return m_obj->begin();
    else
      return m_obj->values_begin();
  }
  auto end() const {
    if constexpr (std::is_same_v<typename T::mapped_type, DataArray>)
      return m_obj->end();
    else
      return m_obj->values_end();
  }

private:
  T *m_obj;
};
template <class T> values_view(T &)->values_view<T>;

/// Helper to provide equivalent of the `keys()` method of a Python dict.
template <class T> class keys_view {
public:
  keys_view(T &obj) : m_obj(&obj) {}
  auto size() const noexcept { return m_obj->size(); }
  auto begin() const { return m_obj->keys_begin(); }
  auto end() const { return m_obj->keys_end(); }

private:
  T *m_obj;
};
template <class T> keys_view(T &)->keys_view<T>;

template <template <class> class View, class T>
void bind_helper_view(py::module &m, const std::string &name) {
  std::string suffix;
  if (std::is_same_v<View<T>, items_view<T>>)
    suffix = "_items_view";
  if (std::is_same_v<View<T>, values_view<T>>)
    suffix = "_values_view";
  if (std::is_same_v<View<T>, keys_view<T>>)
    suffix = "_keys_view";
  py::class_<View<T>>(m, (name + suffix).c_str())
      .def(py::init([](T &obj) { return View{obj}; }))
      .def("__len__", &View<T>::size)
      .def("__iter__",
           [](View<T> &self) {
             return py::make_iterator(self.begin(), self.end(),
                                      py::return_value_policy::move);
           },
           py::return_value_policy::move, py::keep_alive<0, 1>());
}

template <class T, class ConstT>
void bind_mutable_view(py::module &m, const std::string &name) {
  py::class_<ConstT>(m, (name + "ConstView").c_str());
  py::class_<T, ConstT> view(m, (name + "View").c_str());
  view.def("__len__", &T::size)
      .def("__getitem__", &T::operator[], py::return_value_policy::move,
           py::keep_alive<0, 1>())
      .def("__setitem__",
           [](T &self, const typename T::key_type key,
              const VariableConstView &var) { self.set(key, var); })
      // This additional setitem allows us to do things like
      // d.attrs["a"] = scipp.detail.move(scipp.Variable())
      .def("__setitem__",
           [](T &self, const typename T::key_type key, MoveableVariable &mvar) {
             self.set(key, std::move(mvar.var));
           })
      .def("__delitem__", &T::erase, py::call_guard<py::gil_scoped_release>())
      .def("__iter__",
           [](T &self) {
             return py::make_iterator(self.keys_begin(), self.keys_end(),
                                      py::return_value_policy::move);
           },
           py::keep_alive<0, 1>())
      .def("keys", [](T &self) { return keys_view(self); },
           py::keep_alive<0, 1>(), R"(view on self's keys)")
      .def("values", [](T &self) { return values_view(self); },
           py::keep_alive<0, 1>(), R"(view on self's values)")
      .def("items", [](T &self) { return items_view(self); },
           py::return_value_policy::move, py::keep_alive<0, 1>(),
           R"(view on self's items)")
      .def("__contains__", &T::contains);
  bind_comparison<T>(view);
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
      []([[maybe_unused]] T &self) {
        throw std::runtime_error(
            "Property `labels` is deprecated. Use `coords` instead.");
      },
      R"(Decprecated, alias for `coords`.)");
  c.def_property_readonly("masks",
                          py::cpp_function([](T &self) { return self.masks(); },
                                           py::return_value_policy::move,
                                           py::keep_alive<0, 1>()),
                          R"(
      Dict of masks.)");
  c.def_property_readonly("attrs",
                          py::cpp_function([](T &self) { return self.attrs(); },
                                           py::return_value_policy::move,
                                           py::keep_alive<0, 1>()),
                          R"(
      Dict of attributes.)");

  if constexpr (std::is_same_v<T, DataArray> || std::is_same_v<T, Dataset>)
    c.def("realign", [](T &self, py::dict coord_dict) {
      // Python dicts above 3.7 preserve order, but we cannot use
      // automatic conversion by pybind11 since C++ maps do not.
      std::vector<std::pair<Dim, Variable>> coords;
      for (auto item : coord_dict)
        coords.emplace_back(Dim(item.first.cast<std::string>()),
                            item.second.cast<VariableView>());
      self = unaligned::realign(std::move(self), std::move(coords));
    });
}

template <class T, class... Ignored>
void bind_dataset_view_methods(py::class_<T, Ignored...> &c) {
  c.def("__len__", &T::size);
  c.def("__repr__", [](const T &self) { return to_string(self); });
  c.def("__iter__",
        [](const T &self) {
          return py::make_iterator(self.keys_begin(), self.keys_end(),
                                   py::return_value_policy::move);
        },
        py::return_value_policy::move, py::keep_alive<0, 1>());
  c.def("keys", [](T &self) { return keys_view(self); },
        py::return_value_policy::move, py::keep_alive<0, 1>(),
        R"(view on self's keys)");
  c.def("values", [](T &self) { return values_view(self); },
        py::return_value_policy::move, py::keep_alive<0, 1>(),
        R"(view on self's values)");
  c.def("items", [](T &self) { return items_view(self); },
        py::return_value_policy::move, py::keep_alive<0, 1>(),
        R"(view on self's items)");
  c.def("__getitem__",
        [](T &self, const std::string &name) { return self[name]; },
        py::keep_alive<0, 1>());
  c.def("__contains__", &T::contains);
  c.def("copy", [](const T &self) { return Dataset(self); },
        py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def("__copy__", [](const T &self) { return Dataset(self); },
        py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def("__deepcopy__",
        [](const T &self, const py::dict &) { return Dataset(self); },
        py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
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
                            std::vector<int64_t> shape;
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
        py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def("__copy__", [](const T &self) { return DataArray(self); },
        py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def("__deepcopy__",
        [](const T &self, const py::dict &) { return DataArray(self); },
        py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def_property(
      "data",
      py::cpp_function(
          [](T &self) {
            return self.hasData() ? py::cast(self.data()) : py::none();
          },
          py::return_value_policy::move, py::keep_alive<0, 1>()),
      [](T &self, const VariableConstView &data) { self.data().assign(data); },
      R"(Underlying data item.)");
  c.def_property_readonly(
      "unaligned",
      py::cpp_function(
          [](T &self) {
            return self.hasData() ? py::none() : py::cast(self.unaligned());
          },
          py::return_value_policy::move, py::keep_alive<0, 1>()),
      R"(Underlying unaligned data item.)");
  bind_coord_properties(c);
  bind_comparison<DataArrayConstView>(c);
  bind_data_properties(c);
  bind_slice_methods(c);
  bind_in_place_binary<DataArrayView>(c);
  bind_in_place_binary<VariableConstView>(c);
  bind_binary<Dataset>(c);
  bind_binary<DatasetView>(c);
  bind_binary<DataArrayView>(c);
  bind_binary<VariableConstView>(c);
}

template <class T, class... Ignored>
void bind_astype(py::class_<T, Ignored...> &c) {
  c.def("astype",
        [](const T &self, const DType type) { return astype(self, type); },
        py::call_guard<py::gil_scoped_release>(),
        R"(
        Converts a DataArray to a different type.

        :raises: If the variable cannot be converted to the requested dtype.
        :return: New array with specified dtype.
        :rtype: DataArray)");
}

void init_dataset(py::module &m) {
  py::class_<Slice>(m, "Slice");

  bind_helper_view<items_view, Dataset>(m, "Dataset");
  bind_helper_view<items_view, DatasetView>(m, "DatasetView");
  bind_helper_view<items_view, CoordsView>(m, "CoordsView");
  bind_helper_view<items_view, MasksView>(m, "MasksView");
  bind_helper_view<items_view, AttrsView>(m, "AttrsView");
  bind_helper_view<keys_view, Dataset>(m, "Dataset");
  bind_helper_view<keys_view, DatasetView>(m, "DatasetView");
  bind_helper_view<keys_view, CoordsView>(m, "CoordsView");
  bind_helper_view<keys_view, MasksView>(m, "MasksView");
  bind_helper_view<keys_view, AttrsView>(m, "AttrsView");
  bind_helper_view<values_view, Dataset>(m, "Dataset");
  bind_helper_view<values_view, DatasetView>(m, "DatasetView");
  bind_helper_view<values_view, CoordsView>(m, "CoordsView");
  bind_helper_view<values_view, MasksView>(m, "MasksView");
  bind_helper_view<values_view, AttrsView>(m, "AttrsView");

  bind_mutable_view<CoordsView, CoordsConstView>(m, "Coords");
  bind_mutable_view<MasksView, MasksConstView>(m, "Masks");
  bind_mutable_view<AttrsView, AttrsConstView>(m, "Attrs");

  py::class_<DataArray> dataArray(m, "DataArray", R"(
    Named variable with associated coords, masks, and attributes.)");
  dataArray.def(py::init<const DataArrayConstView &>());
  dataArray.def(py::init<Variable, std::map<Dim, Variable>,
                         std::map<std::string, Variable>,
                         std::map<std::string, Variable>>(),
                py::arg("data") = Variable{},
                py::arg("coords") = std::map<Dim, Variable>{},
                py::arg("masks") = std::map<std::string, Variable>{},
                py::arg("attrs") = std::map<std::string, Variable>{});

  py::class_<DataArrayConstView>(m, "DataArrayConstView")
      .def(py::init<const DataArray &>());

  py::class_<DataArrayView, DataArrayConstView> dataArrayView(
      m, "DataArrayView", R"(
        View for DataArray, representing a sliced view onto a DataArray, or an item of a Dataset;
        Mostly equivalent to DataArray, see there for details.)");
  dataArrayView.def(py::init<DataArray &>());

  bind_data_array_properties(dataArray);
  bind_data_array_properties(dataArrayView);

  py::class_<DatasetConstView>(m, "DatasetConstView")
      .def(py::init<const Dataset &>());
  py::class_<DatasetView, DatasetConstView> datasetView(m, "DatasetView",
                                                        R"(
        View for Dataset, representing a sliced view onto a Dataset;
        Mostly equivalent to Dataset, see there for details.)");
  datasetView.def(py::init<Dataset &>());

  py::class_<Dataset> dataset(m, "Dataset", R"(
    Dict of data arrays with aligned dimensions.)");

  dataset.def(py::init<const std::map<std::string, DataArrayConstView> &>())
      .def(py::init<const DataArrayConstView &>())
      .def(py::init([](const std::map<std::string, VariableConstView> &data,
                       const std::map<Dim, VariableConstView> &coords,
                       const std::map<std::string, VariableConstView> &masks,
                       const std::map<std::string, VariableConstView> &attrs) {
             return Dataset(data, coords, masks, attrs);
           }),
           py::arg("data") = std::map<std::string, VariableConstView>{},
           py::arg("coords") = std::map<Dim, VariableConstView>{},
           py::arg("masks") = std::map<std::string, VariableConstView>{},
           py::arg("attrs") = std::map<std::string, VariableConstView>{})
      .def(py::init([](const DatasetView &other) { return Dataset{other}; }))
      .def("__setitem__",
           [](Dataset &self, const std::string &name,
              const VariableConstView &data) { self.setData(name, data); })
      .def("__setitem__",
           [](Dataset &self, const std::string &name, MoveableVariable &mvar) {
             self.setData(name, std::move(mvar.var));
           })
      .def("__setitem__",
           [](Dataset &self, const std::string &name,
              const DataArrayConstView &data) { self.setData(name, data); })
      .def("__setitem__",
           [](Dataset &self, const std::string &name, MoveableDataArray &mdat) {
             self.setData(name, std::move(mdat.data));
           })
      .def("__delitem__", &Dataset::erase,
           py::call_guard<py::gil_scoped_release>())
      .def(
          "clear", &Dataset::clear,
          R"(Removes all data (preserving coordinates, attributes, and masks.).)");
  datasetView.def(
      "__setitem__",
      [](const DatasetView &self, const std::string &name,
         const DataArrayConstView &data) { self[name].assign(data); });

  bind_dataset_view_methods(dataset);
  bind_dataset_view_methods(datasetView);

  bind_coord_properties(dataset);
  bind_coord_properties(datasetView);

  bind_slice_methods(dataset);
  bind_slice_methods(datasetView);

  bind_comparison<Dataset>(dataset);
  bind_comparison<DatasetView>(dataset);
  bind_comparison<Dataset>(datasetView);
  bind_comparison<DatasetView>(datasetView);

  bind_in_place_binary<Dataset>(dataset);
  bind_in_place_binary<DatasetView>(dataset);
  bind_in_place_binary<DataArrayView>(dataset);
  bind_in_place_binary<VariableConstView>(dataset);
  bind_in_place_binary<Dataset>(datasetView);
  bind_in_place_binary<DatasetView>(datasetView);
  bind_in_place_binary<VariableConstView>(datasetView);
  bind_in_place_binary<DataArrayView>(datasetView);
  bind_in_place_binary_scalars(dataset);
  bind_in_place_binary_scalars(datasetView);
  bind_in_place_binary_scalars(dataArray);
  bind_in_place_binary_scalars(dataArrayView);

  bind_binary<Dataset>(dataset);
  bind_binary<DatasetView>(dataset);
  bind_binary<DataArrayView>(dataset);
  bind_binary<VariableConstView>(dataset);
  bind_binary<Dataset>(datasetView);
  bind_binary<DatasetView>(datasetView);
  bind_binary<DataArrayView>(datasetView);
  bind_binary<VariableConstView>(datasetView);

  dataArray.def("rename_dims", &rename_dims<DataArray>, py::arg("dims_dict"),
                "Rename dimensions.");
  dataset.def("rename_dims", &rename_dims<Dataset>, py::arg("dims_dict"),
              "Rename dimensions.");

  m.def("concatenate",
        py::overload_cast<const DataArrayConstView &,
                          const DataArrayConstView &, const Dim>(&concatenate),
        py::arg("x"), py::arg("y"), py::arg("dim"),
        py::call_guard<py::gil_scoped_release>(), R"(
        Concatenate input data array along the given dimension.

        Concatenates the data, coords, and masks of the data array.
        Coords, and masks for any but the given dimension are required to match and are copied to the output without changes.

        :param x: First DataArray.
        :param y: Second DataArray.
        :param dim: Dimension along which to concatenate.
        :raises: If the dtype or unit does not match, or if the dimensions and shapes are incompatible.
        :return: New data array containing all data, coords, and masks of the input arrays.
        :rtype: DataArray)");

  m.def("concatenate",
        py::overload_cast<const DatasetConstView &, const DatasetConstView &,
                          const Dim>(&concatenate),
        py::arg("x"), py::arg("y"), py::arg("dim"),
        py::call_guard<py::gil_scoped_release>(), R"(
        Concatenate input datasets along the given dimension.

        Concatenate all cooresponding items in the input datasets.
        The output contains only items that are present in both inputs.

        :param x: First Dataset.
        :param y: Second Dataset.
        :param dim: Dimension along which to concatenate.
        :raises: If the dtype or unit does not match, or if the dimensions and shapes are incompatible.
        :return: New dataset.
        :rtype: Dataset)");

  m.def("histogram",
        [](const DataArrayConstView &ds, const Variable &bins) {
          return core::histogram(ds, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        R"(Returns a new DataArray with values in bins for sparse dims.

        :param x: Data to histogram.
        :param bins: Bin edges.
        :return: Histogramed data.
        :rtype: DataArray)");

  m.def("histogram",
        [](const DataArrayConstView &ds, const VariableConstView &bins) {
          return core::histogram(ds, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        R"(Returns a new DataArray with values in bins for sparse dims.

        :param x: Data to histogram.
        :param bins: Bin edges.
        :return: Histogramed data.
        :rtype: DataArray)");

  m.def("histogram",
        [](const Dataset &ds, const VariableConstView &bins) {
          return core::histogram(ds, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        R"(Returns a new Dataset with values in bins for sparse dims.

        :param x: Data to histogram.
        :param bins: Bin edges.
        :return: Histogramed data.
        :rtype: Dataset)");

  m.def("histogram",
        [](const Dataset &ds, const Variable &bins) {
          return core::histogram(ds, bins);
        },
        py::arg("x"), py::arg("bins"), py::call_guard<py::gil_scoped_release>(),
        R"(Returns a new Dataset with values in bins for sparse dims.

        :param x: Data to histogram.
        :param bins: Bin edges.
        :return: Histogramed data.
        :rtype: Dataset)");

  m.def("merge",
        [](const DatasetConstView &lhs, const DatasetConstView &rhs) {
          return core::merge(lhs, rhs);
        },
        py::arg("lhs"), py::arg("rhs"),
        py::call_guard<py::gil_scoped_release>(), R"(
        Union of two datasets.

        :param lhs: First Dataset.
        :param rhs: Second Dataset.
        :raises: If there are conflicting items with different content.
        :return: A new dataset that contains the union of all data items, coords, masks and attributes.
        :rtype: Dataset)");

  m.def("sum", py::overload_cast<const DataArrayConstView &, const Dim>(&sum),
        py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Element-wise sum over the specified dimension.

        :param x: Data to sum.
        :param dim: Dimension over which to sum.
        :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New data array containing the sum.
        :rtype: DataArray)");

  m.def("sum", py::overload_cast<const DatasetConstView &, const Dim>(&sum),
        py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Element-wise sum over the specified dimension.

        :param x: Data to sum.
        :param dim: Dimension over which to sum.
        :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New dataset containing the sum for each data item.
        :rtype: Dataset)");

  m.def("mean", py::overload_cast<const DataArrayConstView &, const Dim>(&mean),
        py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Element-wise mean over the specified dimension, if variances are present, the new variance is computated as standard-deviation of the mean.

        See the documentation for the mean of a Variable for details in the computation of the ouput variance.

        :param x: Data to calculate mean of.
        :param dim: Dimension over which to calculate mean.
        :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New data array containing the mean for each data item.
        :rtype: DataArray)");

  m.def("mean", py::overload_cast<const DatasetConstView &, const Dim>(&mean),
        py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Element-wise mean over the specified dimension, if variances are present, the new variance is computated as standard-deviation of the mean.

        See the documentation for the mean of a Variable for details in the computation of the ouput variance.

        :param x: Data to calculate mean of.
        :param dim: Dimension over which to calculate mean.
        :raises: If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New dataset containing the mean for each data item.
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

  m.def(
      "sort",
      py::overload_cast<const DataArrayConstView &, const VariableConstView &>(
          &sort),
      py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      R"(Sort data array along a dimension by a sort key.

        :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
        :return: New sorted data array.
        :rtype: DataArray)");

  m.def(
      "sort", py::overload_cast<const DataArrayConstView &, const Dim &>(&sort),
      py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      R"(Sort data array along a dimension by the coordinate values for that dimension.

      :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
      :return: New sorted data array.
      :rtype: DataArray)");

  m.def("sort",
        py::overload_cast<const DatasetConstView &, const VariableConstView &>(
            &sort),
        py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
        R"(Sort dataset along a dimension by a sort key.

        :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
        :return: New sorted dataset.
        :rtype: Dataset)");

  m.def(
      "sort", py::overload_cast<const DatasetConstView &, const Dim &>(&sort),
      py::arg("x"), py::arg("key"), py::call_guard<py::gil_scoped_release>(),
      R"(Sort dataset along a dimension by the coordinate values for that dimension.

      :raises: If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.
      :return: New sorted dataset.
      :rtype: Dataset)");

  m.def("combine_masks",
        [](const MasksConstView &msk, const std::vector<Dim> &labels,
           const std::vector<scipp::index> &shape) {
          return core::masks_merge_if_contained(
              msk, core::Dimensions(labels, shape));
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

  m.def("reciprocal",
        [](const DataArrayConstView &self) { return reciprocal(self); },
        py::arg("x"), py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise reciprocal.

        :return: Reciprocal of the input values.
        :rtype: DataArray)");

  m.def("realign",
        [](const DataArrayConstView &a, py::dict coord_dict) {
          // Python dicts above 3.7 preserve order, but we cannot use automatic
          // conversion by pybind11 since C++ maps do not.
          std::vector<std::pair<Dim, Variable>> coords;
          for (auto item : coord_dict)
            coords.emplace_back(Dim(item.first.cast<std::string>()),
                                item.second.cast<VariableView>());
          return unaligned::realign(copy(a), std::move(coords));
        },
        py::arg("data"), py::arg("coords"));
  m.def(
      "histogram",
      [](const DataArrayConstView &x) { return core::histogram(x); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      R"(Returns a new DataArray unaligned data content binned according to the realigning axes.

        :param x: Realigned data to histogram.
        :return: Histogramed data.
        :rtype: DataArray)");

  bind_astype(dataArray);
  bind_astype(dataArrayView);

  py::implicitly_convertible<DataArray, DataArrayConstView>();
  py::implicitly_convertible<DataArray, DataArrayView>();
  py::implicitly_convertible<Dataset, DatasetConstView>();
}
