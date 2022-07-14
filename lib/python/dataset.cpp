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

namespace {
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
  bind_common_data_properties(c);
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
  bind_binary_scalars(c);
  bind_reverse_binary_scalars(c);
  bind_comparison<DataArray>(c);
  bind_comparison<Variable>(c);
  bind_unary(c);
  bind_logical<DataArray>(c);
  bind_logical<Variable>(c);
  bind_boolean_unary(c);
}

template <class T> void bind_rebin(py::module &m) {
  m.def(
      "rebin",
      [](const T &x, const std::string &dim, const Variable &bins) {
        return rebin(x, Dim{dim}, bins);
      },
      py::arg("x"), py::arg("dim"), py::arg("bins"),
      py::call_guard<py::gil_scoped_release>());
}

template <class Key, class Value> auto to_cpp_map(const py::dict &dict) {
  std::unordered_map<Key, Value> out;
  for (const auto &[key, val] : dict) {
    out.emplace(key.template cast<std::string>(), val.template cast<Value &>());
  }
  return out;
}
} // namespace

void init_dataset(py::module &m) {
  static_cast<void>(py::class_<Slice>(m, "Slice"));

  bind_helper_view<items_view, Dataset>(m, "Dataset");
  bind_helper_view<str_items_view, Coords>(m, "Coords");
  bind_helper_view<items_view, Masks>(m, "Masks");
  bind_helper_view<keys_view, Dataset>(m, "Dataset");
  bind_helper_view<str_keys_view, Coords>(m, "Coords");
  bind_helper_view<keys_view, Masks>(m, "Masks");
  bind_helper_view<values_view, Dataset>(m, "Dataset");
  bind_helper_view<values_view, Coords>(m, "Coords");
  bind_helper_view<values_view, Masks>(m, "Masks");

  bind_mutable_view_no_dim<Coords>(m, "Coords",
                                   R"(dict-like collection of meta data

Returned by :py:func:`DataArray.coords`, :py:func:`DataArray.attrs`, :py:func:`DataArray.meta`,
and the corresponding properties of :py:class:`Dataset`.)");
  bind_mutable_view<Masks>(m, "Masks", R"(dict-like collection of masks.

Returned by :py:func:`DataArray.masks`)");

  py::class_<DataArray> dataArray(m, "DataArray", R"(
    Named variable with associated coords, masks, and attributes.)");
  py::options options;
  options.disable_function_signatures();
  dataArray.def(
      py::init([](const Variable &data, const py::object &coords,
                  const py::object &masks, const py::object &attrs,
                  const std::string &name) {
        return DataArray{data, to_cpp_map<Dim, Variable>(coords),
                         to_cpp_map<std::string, Variable>(masks),
                         to_cpp_map<Dim, Variable>(attrs), name};
      }),
      py::arg("data"), py::kw_only(), py::arg("coords") = py::dict(),
      py::arg("masks") = py::dict(), py::arg("attrs") = py::dict(),
      py::arg("name") = std::string{},
      R"doc(__init__(self, data: Variable, coords: Union[typing.Mapping[str, Variable], Iterable[Tuple[str, Variable]]] = {}, masks: Union[typing.Mapping[str, Variable], Iterable[Tuple[str, Variable]]] = {}, attrs: Union[typing.Mapping[str, Variable], Iterable[Tuple[str, Variable]]] = {}, name: str = '') -> None

          DataArray initializer.

          Parameters
          ----------
          data:
              Data and optionally variances.
          coords:
              Coordinates referenced by dimension.
          masks:
              Masks referenced by name.
          attrs:
              Attributes referenced by dimension.
          name:
              Name of the data array.
          )doc");
  options.enable_function_signatures();
  dataArray
      .def("__sizeof__",
           [](const DataArray &array) {
             return size_of(array, SizeofTag::ViewOnly, true);
           })
      .def("underlying_size", [](const DataArray &self) {
        return size_of(self, SizeofTag::Underlying);
      });

  bind_data_array(dataArray);

  py::class_<Dataset> dataset(m, "Dataset", R"(
  Dict of data arrays with aligned dimensions.)");
  options.disable_function_signatures();
  dataset.def(
      py::init([](const py::object &data, const py::object &coords) {
        Dataset d;
        for (auto &&[name, item] : py::dict(data)) {
          if (py::isinstance<DataArray>(item)) {
            d.setData(name.cast<std::string>(), item.cast<DataArray &>());
          } else {
            d.setData(name.cast<std::string>(), item.cast<Variable &>());
          }
        }
        for (auto &&[dim, coord] : py::dict(coords))
          d.setCoord(Dim{dim.cast<std::string>()}, coord.cast<Variable &>());
        return d;
      }),
      py::arg("data") = py::dict(), py::kw_only(),
      py::arg("coords") = std::map<std::string, Variable>{},
      R"doc(__init__(self, data: Union[typing.Mapping[str, Union[Variable, DataArray]], Iterable[Tuple[str, Union[Variable, DataArray]]]] = {}, coords: Union[typing.Mapping[str, Variable], Iterable[Tuple[str, Variable]]] = {}) -> None

      Dataset initializer.

      Parameters
      ----------
      data:
          Dictionary of name and data pairs.
      coords:
          Dictionary of name and coord pairs.
      )doc");
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
  bind_dict_update(dataset,
                   [](Dataset &self, const std::string &key,
                      const DataArray &value) { self.setData(key, value); });

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

  dataArray.def("_rename_dims", &rename_dims<DataArray>);
  dataset.def("_rename_dims", &rename_dims<Dataset>);

  m.def(
      "merge",
      [](const Dataset &lhs, const Dataset &rhs) {
        return dataset::merge(lhs, rhs);
      },
      py::arg("lhs"), py::arg("rhs"), py::call_guard<py::gil_scoped_release>());

  m.def(
      "irreducible_mask",
      [](const Masks &masks, const std::string &dim) {
        py::gil_scoped_release release;
        auto mask = irreducible_mask(masks, Dim{dim});
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
