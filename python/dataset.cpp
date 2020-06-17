// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/map_view.h"
#include "scipp/dataset/math.h"
#include "scipp/dataset/rebin.h"
#include "scipp/dataset/sort.h"
#include "scipp/dataset/unaligned.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "detail.h"
#include "docstring.h"
#include "pybind11.h"
#include "rename.h"
#include "view.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

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
      .def(
          "__iter__",
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
              const VariableConstView &var) {
             if (self.contains(key) &&
                 self[key].dims().ndim() == var.dims().ndim() &&
                 self[key].dims().contains(var.dims()))
               self[key].assign(var);
             else
               self.set(key, var);
           })
      // This additional setitem allows us to do things like
      // d.attrs["a"] = scipp.detail.move(scipp.Variable())
      .def("__setitem__",
           [](T &self, const typename T::key_type key, MoveableVariable &mvar) {
             self.set(key, std::move(mvar.var));
           })
      .def("__delitem__", &T::erase, py::call_guard<py::gil_scoped_release>())
      .def(
          "__iter__",
          [](T &self) {
            return py::make_iterator(self.keys_begin(), self.keys_end(),
                                     py::return_value_policy::move);
          },
          py::keep_alive<0, 1>())
      .def(
          "keys", [](T &self) { return keys_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's keys)")
      .def(
          "values", [](T &self) { return values_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's values)")
      .def(
          "items", [](T &self) { return items_view(self); },
          py::return_value_policy::move, py::keep_alive<0, 1>(),
          R"(view on self's items)")
      .def("__contains__", &T::contains);
  bind_comparison<T>(view);
}

template <class T> void realign_impl(T &self, py::dict coord_dict) {
  // Python dicts above 3.7 preserve order, but we cannot use
  // automatic conversion by pybind11 since C++ maps do not.
  std::vector<std::pair<Dim, Variable>> coords;
  // Cast to VariableView uses implicit conversion and causes segfault if
  // GIL is released.
  for (auto item : coord_dict)
    coords.emplace_back(Dim(item.first.cast<std::string>()),
                        item.second.cast<VariableView>());
  self = unaligned::realign(std::move(self), std::move(coords));
}

template <class T>
T filter_impl(const typename T::const_view_type &self, const Dim dim,
              const VariableConstView &interval, bool keep_attrs) {
  return event::filter(self, dim, interval,
                       keep_attrs ? AttrPolicy::Keep : AttrPolicy::Drop);
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
    c.def("realign", realign_impl<T>);
}

template <class T, class... Ignored>
void bind_dataset_view_methods(py::class_<T, Ignored...> &c) {
  c.def("__len__", &T::size);
  c.def("__repr__", [](const T &self) { return to_string(self); });
  c.def(
      "__iter__",
      [](const T &self) {
        return py::make_iterator(self.keys_begin(), self.keys_end(),
                                 py::return_value_policy::move);
      },
      py::return_value_policy::move, py::keep_alive<0, 1>());
  c.def(
      "keys", [](T &self) { return keys_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(view on self's keys)");
  c.def(
      "values", [](T &self) { return values_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(view on self's values)");
  c.def(
      "items", [](T &self) { return items_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(view on self's items)");
  c.def(
      "__getitem__",
      [](T &self, const std::string &name) { return self[name]; },
      py::keep_alive<0, 1>());
  c.def("__contains__", &T::contains);
  c.def(
      "copy", [](const T &self) { return Dataset(self); },
      py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def(
      "__copy__", [](const T &self) { return Dataset(self); },
      py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def(
      "__deepcopy__",
      [](const T &self, const py::dict &) { return Dataset(self); },
      py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def_property_readonly(
      "dims",
      [](const T &self) {
        std::vector<Dim> dims;
        for (const auto &dim : self.dimensions()) {
          dims.push_back(dim.first);
        }
        return dims;
      },
      R"(List of dimensions.)", py::return_value_policy::move);
  c.def_property_readonly(
      "shape",
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
  c.def(
      "copy", [](const T &self) { return DataArray(self); },
      py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def(
      "__copy__", [](const T &self) { return DataArray(self); },
      py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
  c.def(
      "__deepcopy__",
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
  c.def(
      "astype",
      [](const T &self, const DType type) { return astype(self, type); },
      py::call_guard<py::gil_scoped_release>(),
      R"(
        Converts a DataArray to a different type.

        :raises: If the variable cannot be converted to the requested dtype.
        :return: New array with specified dtype.
        :rtype: DataArray)");
}

template <class T> void bind_rebin(py::module &m) {
  m.def("rebin",
        py::overload_cast<const typename T::const_view_type &, const Dim,
                          const VariableConstView &>(&rebin),
        py::arg("x"), py::arg("dim"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>(),
        Docstring()
            .description("Rebin a dimension of a data array.")
            .raises("If data cannot be rebinned, e.g., if the unit is not "
                    "counts, or the existing coordinate is not a bin-edge "
                    "coordinate.")
            .returns("Data rebinned according to the new coordinate.")
            .rtype<T>()
            .template param<T>("x", "Data to rebin.")
            .param("dim", "Dimension to rebin over.", "Dim")
            .param("bins", "New bin edges.", "Variable")
            .c_str());
}

template <class T> void bind_realign(py::module &m) {
  // Note: adding `py::call_guard<py::gil_scoped_release>()` for this binding
  // causes a segmentation fault.
  m.def(
      "realign",
      [](const typename T::const_view_type &a, py::dict coord_dict) {
        T copy(a);
        realign_impl(copy, coord_dict);
        return copy;
      },
      py::arg("data"), py::arg("coords"),
      Docstring()
          .description("Realign unaligned data to the supplied coordinate "
                       "axes.")
          .raises("If the input does not contain unaligned data.")
          .returns("A data structure containing unaligned underlying data, "
                   "along with coordinate axes for alignment.")
          .rtype<T>()
          .template param<T>("x", "Unaligned data to realign.")
          .param("coords", "Coordinates for re-alignment.", "Dict")
          .c_str());
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
  dataArray.def(py::init([](VariableConstView data,
                            std::map<Dim, VariableConstView> coords,
                            std::map<std::string, VariableConstView> masks,
                            std::map<std::string, VariableConstView> attrs) {
                  return DataArray{Variable{data}, coords, masks, attrs};
                }),
                py::arg("data") = Variable{},
                py::arg("coords") = std::map<Dim, VariableConstView>{},
                py::arg("masks") = std::map<std::string, VariableConstView>{},
                py::arg("attrs") = std::map<std::string, VariableConstView>{});

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

  m.def(
      "merge",
      [](const DatasetConstView &lhs, const DatasetConstView &rhs) {
        return dataset::merge(lhs, rhs);
      },
      py::arg("lhs"), py::arg("rhs"), py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("Union of two datasets.")
          .raises("If there are conflicting items with different content.")
          .returns("A new dataset that contains the union of all data items, "
                   "coords, masks and attributes.")
          .rtype("Dataset")
          .param("lhs", "First Dataset", "Dataset")
          .param("rhs", "Second Dataset", "Dataset")
          .c_str());

  m.def(
      "combine_masks",
      [](const MasksConstView &msk, const std::vector<Dim> &labels,
         const std::vector<scipp::index> &shape) {
        return dataset::masks_merge_if_contained(msk,
                                                 Dimensions(labels, shape));
      },
      py::arg("masks"), py::arg("labels"), py::arg("shape"),
      py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description(
              "Combine all masks into a single one following the OR operation. "
              "This requires a masks view as an input, followed by the "
              "dimension labels and shape of the Variable/DataArray. The "
              "labels and the shape are used to create a Dimensions object. "
              "The function then iterates through the masks view and combines "
              "only the masks that have all their dimensions contained in the "
              "Variable/DataArray Dimensions.")
          .returns("A new variable that contains the union of all masks.")
          .rtype("Variable")
          .param("masks", "Masks view of the dataset's masks.", "MaskView")
          .param("labels", "A list of dimension labels.", "list")
          .param("shape", "A list of dimension extents.", "list")
          .c_str());

  m.def(
      "reciprocal",
      [](const DataArrayConstView &self) { return reciprocal(self); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("Element-wise reciprocal.")
          .raises("If the dtype has no reciprocal, e.g., if it is a string.")
          .returns("The reciprocal values of the input.")
          .rtype("DataArray")
          .param("x", "Input data array.", "DataArray")
          .c_str());

  m.def("filter", filter_impl<DataArray>, py::arg("data"), py::arg("filter"),
        py::arg("interval"), py::arg("keep_attrs") = true,
        py::call_guard<py::gil_scoped_release>(),
        Docstring()
            .description(
                "Return filtered event data. This only supports event data.")
            .returns("Filtered data.")
            .rtype("DataArray")
            .param("data", "Input event data.", "DataArray")
            .param("filter", "Name of coord to use for filtering.", "str")
            .param("interval",
                   "Variable defining the valid interval of coord values to "
                   "include in the output.",
                   "Variable")
            .param("keep_attrs",
                   "If `False`, attributes are not copied to the output, "
                   "default is `True`.",
                   "bool")
            .c_str());

  m.def("map", event::map, py::arg("function"), py::arg("iterable"),
        py::arg("dim") = to_string(Dim::Invalid),
        py::call_guard<py::gil_scoped_release>(),
        Docstring()
            .description(
                "Return mapped event data. This only supports event data.")
            .returns("Mapped event data.")
            .rtype("Variable")
            .param("function",
                   "Data array serving as a discretized mapping function.",
                   "DataArray")
            .param("iterable",
                   "Variable with values to map, must be event data.",
                   "Variable")
            .param("dim",
                   "Optional dimension to use for mapping, if not given, `map` "
                   "will attempt to determine the dimension from the "
                   "`function` argument.",
                   "Dim")
            .c_str());

  bind_astype(dataArray);
  bind_astype(dataArrayView);

  bind_rebin<DataArray>(m);
  bind_rebin<Dataset>(m);

  bind_realign<DataArray>(m);
  bind_realign<Dataset>(m);

  py::implicitly_convertible<DataArray, DataArrayConstView>();
  py::implicitly_convertible<DataArray, DataArrayView>();
  py::implicitly_convertible<Dataset, DatasetConstView>();
}
