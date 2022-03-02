// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/map_view.h"
#include "scipp/dataset/math.h"
#include "scipp/dataset/rebin.h"
#include "scipp/dataset/util.h"

#include "bind_data_access.h"
#include "bind_data_array.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "pybind11.h"
#include "rename.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T, class... Ignored>
void bind_dataset_coord_properties(py::class_<T, Ignored...> &c) {
  // TODO does this comment still apply?
  // For some reason the return value policy and/or keep-alive policy do not
  // work unless we wrap things in py::cpp_function.
  c.def_property_readonly(
      "coords", [](T &self) -> decltype(auto) { return self.coords(); },
      R"(
      Dict of coordinates.)");
  // Metadata for dataset is same as `coords` since dataset cannot have attrs
  // (unaligned coords).
  c.def_property_readonly(
      "meta", [](T &self) -> decltype(auto) { return self.meta(); },
      R"(
      Dict of coordinates.)");
}

template <class T, class... Ignored>
void bind_dataset_view_methods(py::class_<T, Ignored...> &c) {
  bind_common_operators(c);
  c.def("__len__", &T::size);
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
  c.def("__getitem__",
        [](const T &self, const std::string &name) { return self[name]; });
  c.def("__contains__", &T::contains);
  c.def("_ipython_key_completions_", [](T &self) {
    py::list out;
    const auto end = self.keys_end();
    for (auto it = self.keys_begin(); it != end; ++it) {
      out.append(*it);
    }
    return out;
  });
  c.def_property_readonly(
      "dims",
      [](const T &self) {
        std::vector<std::string> dims;
        for (const auto &dim : self.sizes()) {
          dims.push_back(dim.name());
        }
        return dims;
      },
      R"(List of dimensions.)", py::return_value_policy::move);
  // TODO should this be removed?
  c.def_property_readonly(
      "shape",
      [](const T &self) {
        std::vector<int64_t> shape;
        for (const auto &size : self.sizes().sizes()) {
          shape.push_back(size);
        }
        return shape;
      },
      R"(List of shapes.)", py::return_value_policy::move);
  c.def_property_readonly(
      "dim", [](const T &self) { return self.dim().name(); },
      "The only dimension label for 1-dimensional data, raising an exception "
      "if the data is not 1-dimensional.");
  bind_pop(c);
}

template <class T, class... Ignored>
void bind_data_array(py::class_<T, Ignored...> &c) {
  bind_data_array_properties(c);
  bind_common_operators(c);
  bind_data_properties(c);
  bind_slice_methods(c);
  bind_in_place_binary<DataArray>(c);
  bind_in_place_binary<Variable>(c);
  bind_binary<Dataset>(c);
  bind_binary<DataArray>(c);
  bind_binary<Variable>(c);
  bind_comparison<DataArray>(c);
  bind_comparison<Variable>(c);
  bind_unary(c);
  bind_logical<DataArray>(c);
  bind_logical<Variable>(c);
}

template <class T> void bind_rebin(py::module &m) {
  m.def("rebin",
        py::overload_cast<const T &, const Dim, const Variable &>(&rebin),
        py::arg("x"), py::arg("dim"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>());
}

void init_dataset(py::module &m) {
  py::class_<Slice>(m, "Slice");

  bind_helper_view<items_view, Dataset>(m, "Dataset");
  bind_helper_view<str_items_view, Coords>(m, "Coords");
  bind_helper_view<items_view, Masks>(m, "Masks");
  bind_helper_view<keys_view, Dataset>(m, "Dataset");
  bind_helper_view<str_keys_view, Coords>(m, "Coords");
  bind_helper_view<keys_view, Masks>(m, "Masks");
  bind_helper_view<values_view, Dataset>(m, "Dataset");
  bind_helper_view<values_view, Coords>(m, "Coords");
  bind_helper_view<values_view, Masks>(m, "Masks");

  bind_mutable_view_no_dim<Coords>(m, "Coords");
  bind_mutable_view<Masks>(m, "Masks");

  py::class_<DataArray> dataArray(m, "DataArray", R"(
    Named variable with associated coords, masks, and attributes.)");
  py::options options;
  options.disable_function_signatures();
  dataArray
      .def(
          py::init([](const Variable &data,
                      std::unordered_map<Dim, Variable> coords,
                      std::unordered_map<std::string, Variable> masks,
                      std::unordered_map<Dim, Variable> attrs,
                      const std::string &name) {
            return DataArray{data, std::move(coords), std::move(masks),
                             std::move(attrs), name};
          }),
          py::arg("data"), py::kw_only(),
          py::arg("coords") = std::unordered_map<Dim, Variable>{},
          py::arg("masks") = std::unordered_map<std::string, Variable>{},
          py::arg("attrs") = std::unordered_map<Dim, Variable>{},
          py::arg("name") = std::string{},
          R"(__init__(self, data: Variable, coords: Dict[str, Variable] = {}, masks: Dict[str, Variable] = {}, attrs: Dict[str, Variable] = {}, name: str = '') -> None

          DataArray initializer.

          :param data: Data and optionally variances.
          :param coords: Coordinates referenced by dimension.
          :param masks: Masks referenced by name.
          :param attrs: Attributes referenced by dimension.
          :param name: Name of DataArray.
          :type data: Variable
          :type coords: Dict[str, Variable]
          :type masks: Dict[str, Variable]
          :type attrs: Dict[str, Variable]
          :type name: str
          )")
      .def("__sizeof__",
           [](const DataArray &array) {
             return size_of(array, SizeofTag::ViewOnly, true);
           })
      .def("underlying_size", [](const DataArray &self) {
        return size_of(self, SizeofTag::Underlying);
      });
  options.enable_function_signatures();

  bind_data_array(dataArray);

  py::class_<Dataset> dataset(m, "Dataset", R"(
  Dict of data arrays with aligned dimensions.)");

  dataset.def(
      py::init([](const std::map<std::string, std::variant<Variable, DataArray>>
                      &data,
                  const std::map<Dim, Variable> &coords) {
        Dataset d;
        for (auto &&[name, item] : data) {
          auto visitor = [&d, n = name](auto &object) {
            d.setData(std::string(n), std::move(object));
          };
          std::visit(visitor, item);
        }
        for (auto &&[dim, coord] : coords)
          d.setCoord(dim, coord);
        return d;
      }),
      py::arg("data") =
          std::map<std::string, std::variant<Variable, DataArray>>{},
      py::kw_only(), py::arg("coords") = std::map<Dim, Variable>{},
      R"(__init__(self, data: Dict[str, Union[Variable, DataArray]] = {}, coords: Dict[str, Variable] = {}) -> None

              Dataset initializer.

             :param data: Dictionary of name and data pairs.
             :param coords: Dictionary of name and coord pairs.
             :type data: Dict[str, Union[Variable, DataArray]]
             :type coords: Dict[str, Variable]
             )");
  options.enable_function_signatures();

  dataset
      .def("__setitem__",
           [](Dataset &self, const std::string &name, const Variable &data) {
             self.setData(name, data);
           })
      .def("__setitem__",
           [](Dataset &self, const std::string &name, const DataArray &data) {
             self.setData(name, data);
           })
      .def("__delitem__", &Dataset::erase,
           py::call_guard<py::gil_scoped_release>())
      .def("clear", &Dataset::clear,
           R"(Removes all data, preserving coordinates.)")
      .def("__sizeof__",
           [](const Dataset &self) {
             return size_of(self, SizeofTag::ViewOnly);
           })
      .def("underlying_size", [](const Dataset &self) {
        return size_of(self, SizeofTag::Underlying);
      });

  bind_dataset_view_methods(dataset);

  bind_dataset_coord_properties(dataset);

  bind_slice_methods(dataset);

  bind_in_place_binary<Dataset>(dataset);
  bind_in_place_binary<DataArray>(dataset);
  bind_in_place_binary<Variable>(dataset);
  bind_in_place_binary_scalars(dataset);
  bind_in_place_binary_scalars(dataArray);

  bind_binary<Dataset>(dataset);
  bind_binary<DataArray>(dataset);
  bind_binary<Variable>(dataset);

  dataArray.def("rename_dims", &rename_dims<DataArray>, py::arg("dims_dict"),
                py::pos_only(), "Rename dimensions.");
  dataset.def("rename_dims", &rename_dims<Dataset>, py::arg("dims_dict"),
              py::pos_only(), "Rename dimensions.");

  m.def(
      "merge",
      [](const Dataset &lhs, const Dataset &rhs) {
        return dataset::merge(lhs, rhs);
      },
      py::arg("lhs"), py::arg("rhs"), py::call_guard<py::gil_scoped_release>());

  m.def(
      "irreducible_mask",
      [](const Masks &masks, const Dim dim) {
        py::gil_scoped_release release;
        auto mask = irreducible_mask(masks, dim);
        py::gil_scoped_acquire acquire;
        return mask.is_valid() ? py::cast(mask) : py::none();
      },
      py::arg("masks"), py::arg("dim"));

  m.def(
      "reciprocal", [](const DataArray &self) { return reciprocal(self); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>());

  bind_astype(dataArray);

  bind_rebin<DataArray>(m);
  bind_rebin<Dataset>(m);
}
