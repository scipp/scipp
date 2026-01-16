// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/math.h"
#include "scipp/dataset/rebin.h"
#include "scipp/dataset/sized_dict.h"

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
void bind_dataset_properties(py::class_<T, Ignored...> &c) {
  c.def("drop_coords", [](T &self, const std::string &coord_name) {
    std::vector<scipp::Dim> coord_names_c = {scipp::Dim{coord_name}};
    return self.drop_coords(coord_names_c);
  });
  c.def("drop_coords",
        [](T &self, const std::vector<std::string> &coord_names) {
          std::vector<scipp::Dim> coord_names_c;
          std::transform(coord_names.begin(), coord_names.end(),
                         std::back_inserter(coord_names_c),
                         [](const auto &name) { return scipp::Dim{name}; });
          return self.drop_coords(coord_names_c);
        });
}

template <class T, class... Ignored>
void bind_dataset_coord_properties(py::class_<T, Ignored...> &c) {
  // TODO does this comment still apply?
  // For some reason the return value policy and/or keep-alive policy do not
  // work unless we wrap things in py::cpp_function.
  c.def_property_readonly(
      "coords", [](T &self) -> decltype(auto) { return self.coords(); },
      R"(
      Dict of coordinates.)");
}

template <class... Ignored>
void bind_dataset_view_methods(py::class_<Dataset, Ignored...> &c) {
  bind_common_operators(c);
  c.def("__len__", &Dataset::size);
  c.def(
      "__iter__",
      [](const Dataset &self) {
        return py::make_iterator(self.keys_begin(), self.keys_end(),
                                 py::return_value_policy::move);
      },
      py::return_value_policy::move, py::keep_alive<0, 1>());
  c.def(
      "keys", [](Dataset &self) { return keys_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(View of the Dataset's data array names.

Examples
--------

  >>> import scipp as sc
  >>> ds = sc.Dataset({'a': sc.array(dims=['x'], values=[1, 2]),
  ...                  'b': sc.array(dims=['x'], values=[3, 4])})
  >>> list(ds.keys())
  ['a', 'b']
)");
  c.def(
      "values", [](Dataset &self) { return values_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(View of the Dataset's data arrays.

Examples
--------

  >>> import scipp as sc
  >>> ds = sc.Dataset({'a': sc.array(dims=['x'], values=[1, 2]),
  ...                  'b': sc.array(dims=['x'], values=[3, 4])})
  >>> for da in ds.values():
  ...     print(da.dims)
  ('x',)
  ('x',)
)");
  c.def(
      "items", [](Dataset &self) { return items_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(View of the Dataset's (name, data array) pairs.

Examples
--------

  >>> import scipp as sc
  >>> ds = sc.Dataset({'a': sc.array(dims=['x'], values=[1, 2]),
  ...                  'b': sc.array(dims=['x'], values=[3, 4])})
  >>> for name, da in ds.items():
  ...     print(f'{name}: {da.dims}')
  a: ('x',)
  b: ('x',)
)");
  c.def(
      "__getitem__",
      [](const Dataset &self, const std::string &name) { return self[name]; },
      py::arg("name"),
      R"(Access a data item by name.

Parameters
----------
name:
    Name of the data item to access.

Returns
-------
:
    The DataArray with the given name.

Examples
--------
Access a data item in the dataset:

  >>> import scipp as sc
  >>> ds = sc.Dataset({
  ...     'a': sc.array(dims=['x'], values=[1, 2, 3]),
  ...     'b': sc.array(dims=['x'], values=[4.0, 5.0, 6.0], unit='m')
  ... })
  >>> ds['a']
  <scipp.DataArray>
  Dimensions: Sizes[x:3, ]
  Data:
    a                           int64  [dimensionless]  (x)  [1, 2, 3]

  >>> ds['b'].unit
  Unit(m)
)");
  c.def("__contains__", [](const Dataset &self, const py::handle &key) {
    try {
      return self.contains(key.cast<std::string>());
    } catch (py::cast_error &) {
      return false; // if `key` is not a string, it cannot be contained
    }
  });
  c.def("_ipython_key_completions_", [](Dataset &self) {
    py::typing::List<py::str> out;
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
  bind_comparison_scalars(c);
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

template <class Key, class Value> auto to_cpp_dict(const py::dict &dict) {
  core::Dict<Key, Value> out;
  for (const auto &[key, val] : dict) {
    out.insert_or_assign(Key{key.template cast<std::string>()},
                         val.template cast<Value &>());
  }
  return out;
}

auto dataset_from_data_and_coords(const py::dict &data,
                                  const py::dict &coords) {
  Dataset d;
  for (auto &&[name, item] : data) {
    if (py::isinstance<DataArray>(item)) {
      d.setDataInit(name.cast<std::string>(), item.cast<DataArray &>());
    } else {
      d.setDataInit(name.cast<std::string>(), item.cast<Variable &>());
    }
  }
  if (d.is_valid()) {
    // Need to use dataset_from_coords when there is no data to initialize
    // dimensions properly.
    for (auto &&[dim, coord] : coords)
      d.setCoord(Dim{dim.cast<std::string>()}, coord.cast<Variable &>());
  }
  return d;
}

auto dataset_from_coords(const py::dict &py_coords) {
  typename Coords::holder_type coords;
  for (auto &&[dim, coord] : py_coords)
    coords.insert_or_assign(Dim{dim.cast<std::string>()},
                            coord.cast<Variable &>());
  return Dataset({}, std::move(coords));
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
                                   R"(dict-like collection of coordinates.

Returned by :py:meth:`DataArray.coords` and :py:meth:`Dataset.coords`.)");
  bind_mutable_view<Masks>(m, "Masks", R"(dict-like collection of masks.

Returned by :py:func:`DataArray.masks`)");

  py::class_<DataArray> dataArray(m, "DataArray", R"(
Named variable with associated coords and masks.

DataArrays support rich indexing with dimension labels and coordinate values:

Examples
--------
Create a data array and access elements:

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0], unit='K'),
  ...     coords={'x': sc.array(dims=['x'], values=[0.0, 1.0, 2.0, 3.0], unit='m')},
  ... )

Integer indexing with explicit dimension:

  >>> da['x', 0]
  <scipp.DataArray>
  Dimensions: Sizes[]
  Coordinates:
    x                         float64              [m]  ()  0
  Data:
                              float64              [K]  ()  1

Slicing preserves coordinates:

  >>> da['x', 1:3]
  <scipp.DataArray>
  Dimensions: Sizes[x:2, ]
  Coordinates:
  * x                         float64              [m]  (x)  [1, 2]
  Data:
                              float64              [K]  (x)  [2, 3]

Label-based indexing with a scalar value:

  >>> da['x', sc.scalar(1.0, unit='m')]
  <scipp.DataArray>
  Dimensions: Sizes[]
  Coordinates:
    x                         float64              [m]  ()  1
  Data:
                              float64              [K]  ()  2

Label-based slicing with a range:

  >>> da['x', sc.scalar(0.5, unit='m'):sc.scalar(2.5, unit='m')]
  <scipp.DataArray>
  Dimensions: Sizes[x:2, ]
  Coordinates:
  * x                         float64              [m]  (x)  [1, 2]
  Data:
                              float64              [K]  (x)  [2, 3]

Boolean masking:

  >>> condition = da.coords['x'] > sc.scalar(1.0, unit='m')
  >>> da[condition]
  <scipp.DataArray>
  Dimensions: Sizes[x:2, ]
  Coordinates:
  * x                         float64              [m]  (x)  [2, 3]
  Data:
                              float64              [K]  (x)  [3, 4]

See Also
--------
scipp.Variable, scipp.Dataset
)");
  py::options options;
  options.disable_function_signatures();
  dataArray.def(
      py::init([](const Variable &data, const py::object &coords,
                  const py::object &masks, const std::string &name) {
        return DataArray{data, to_cpp_dict<Dim, Variable>(coords),
                         to_cpp_dict<std::string, Variable>(masks), name};
      }),
      py::arg("data"), py::kw_only(), py::arg("coords") = py::dict(),
      py::arg("masks") = py::dict(), py::arg("name") = std::string{},
      R"doc(__init__(self, data: Variable, coords: Union[Mapping[str, Variable], Iterable[tuple[str, Variable]]] = {}, masks: Union[Mapping[str, Variable], Iterable[tuple[str, Variable]]] = {}, name: str = '') -> None

          DataArray initializer.

          Parameters
          ----------
          data:
              Data and optionally variances.
          coords:
              Coordinates referenced by dimension.
          masks:
              Masks referenced by name.
          name:
              Name of the data array.

          Examples
          --------
          Create a DataArray with data and a coordinate:

            >>> import scipp as sc
            >>> da = sc.DataArray(
            ...     data=sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='K'),
            ...     coords={'x': sc.array(dims=['x'], values=[0.1, 0.2, 0.3], unit='m')},
            ... )
            >>> da.dims, da.shape
            (('x',), (3,))
            >>> da.unit == sc.Unit('K')
            True
          )doc");
  options.enable_function_signatures();

  bind_data_array(dataArray);

  py::class_<Dataset> dataset(m, "Dataset", R"(
  Dict of data arrays with aligned dimensions.

A Dataset groups multiple DataArrays that share common coordinates. Operations
on a Dataset apply to all contained DataArrays, and slicing preserves shared
coordinates.

Examples
--------
Create a Dataset with shared coordinates:

  >>> import scipp as sc
  >>> ds = sc.Dataset(
  ...     data={
  ...         'temperature': sc.array(dims=['x'], values=[20.0, 21.0, 22.0], unit='K'),
  ...         'pressure': sc.array(dims=['x'], values=[1.0, 1.1, 1.2], unit='bar'),
  ...     },
  ...     coords={'x': sc.array(dims=['x'], values=[0.0, 1.0, 2.0], unit='m')},
  ... )

Slice a Dataset by dimension (applies to all data arrays):

  >>> ds['x', 0]
  <scipp.Dataset>
  Dimensions: Sizes[]
  Coordinates:
    x                         float64              [m]  ()  0
  Data:
    pressure                  float64            [bar]  ()  1
    temperature               float64              [K]  ()  20

  >>> ds['x', 1:3]
  <scipp.Dataset>
  Dimensions: Sizes[x:2, ]
  Coordinates:
  * x                         float64              [m]  (x)  [1, 2]
  Data:
    pressure                  float64            [bar]  (x)  [1.1, 1.2]
    temperature               float64              [K]  (x)  [21, 22]

Broadcasting operations across all data arrays:

  >>> result = ds + ds  # adds corresponding arrays
  >>> result['temperature'].values
  array([40., 42., 44.])

See Also
--------
scipp.DataArray, scipp.Variable
)");
  options.disable_function_signatures();
  dataset.def(
      py::init([](const py::object &data, const py::object &coords) {
        if (data.is_none() && coords.is_none())
          throw py::type_error("Dataset needs data or coordinates or both.");
        const auto data_dict = data.is_none() ? py::dict() : py::dict(data);
        const auto coords_dict =
            coords.is_none() ? py::dict() : py::dict(coords);
        auto d = dataset_from_data_and_coords(data_dict, coords_dict);
        return d.is_valid() ? d : dataset_from_coords(coords_dict);
      }),
      py::arg("data") = py::none{}, py::kw_only(),
      py::arg("coords") = py::none{},
      R"doc(__init__(self, data: Union[Mapping[str, Union[Variable, DataArray]], Iterable[tuple[str, Union[Variable, DataArray]]]] = {}, coords: Union[Mapping[str, Variable], Iterable[tuple[str, Variable]]] = {}) -> None

      Dataset initializer.

      Parameters
      ----------
      data:
          Dictionary of name and data pairs.
      coords:
          Dictionary of name and coord pairs.

      Examples
      --------
      Create a Dataset with two data arrays and a shared coordinate:

        >>> import scipp as sc
        >>> ds = sc.Dataset(
        ...     data={
        ...         'a': sc.array(dims=['x'], values=[1, 2, 3]),
        ...         'b': sc.array(dims=['x'], values=[4, 5, 6]),
        ...     },
        ...     coords={'x': sc.array(dims=['x'], values=[0.1, 0.2, 0.3], unit='m')},
        ... )
        >>> 'a' in ds
        True
        >>> ds['a'].dims
        ('x',)
        >>> ds.coords['x'].unit == sc.Unit('m')
        True
      )doc");
  options.enable_function_signatures();

  dataset
      .def(
          "__setitem__",
          [](Dataset &self, const std::string &name, const Variable &data) {
            self.setData(name, data);
          },
          py::arg("name"), py::arg("data"),
          R"(Set or add a data item in the dataset.

Parameters
----------
name:
    Name of the data item.
data:
    Variable or DataArray to set.

Examples
--------
Add a new item to the dataset:

  >>> import scipp as sc
  >>> ds = sc.Dataset({'a': sc.array(dims=['x'], values=[1, 2, 3])})
  >>> ds['b'] = sc.array(dims=['x'], values=[4.0, 5.0, 6.0])
  >>> list(ds.keys())
  ['a', 'b']

Overwrite an existing item:

  >>> ds['a'] = sc.array(dims=['x'], values=[10, 20, 30])
  >>> ds['a'].values
  array([10, 20, 30])

Add a DataArray with coordinates:

  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[7, 8, 9]),
  ...     coords={'x': sc.arange('x', 3)}
  ... )
  >>> ds['c'] = da
)")
      .def(
          "__setitem__",
          [](Dataset &self, const std::string &name, const DataArray &data) {
            self.setData(name, data);
          },
          py::arg("name"), py::arg("data"))
      .def("__delitem__", &Dataset::erase,
           py::call_guard<py::gil_scoped_release>())
      .def("clear", &Dataset::clear,
           R"(Removes all data, preserving coordinates.)");

  bind_dataset_view_methods(dataset);
  bind_dict_update(dataset,
                   [](Dataset &self, const std::string &key,
                      const DataArray &value) { self.setData(key, value); });

  bind_dataset_coord_properties(dataset);
  bind_dataset_properties(dataset);

  bind_slice_methods(dataset);

  bind_in_place_binary<Dataset>(dataset);
  bind_in_place_binary<DataArray>(dataset);
  bind_in_place_binary<Variable>(dataset);
  bind_in_place_binary_scalars(dataset);
  bind_in_place_binary_scalars(dataArray);

  bind_binary<Dataset>(dataset);
  bind_binary<DataArray>(dataset);
  bind_binary<Variable>(dataset);
  bind_binary_scalars(dataset);

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
