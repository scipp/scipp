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
#include "nanobind.h"
#include "rename.h"

using namespace scipp;
using namespace scipp::dataset;

namespace nb = nanobind;

namespace {
template <class T, class... Ignored>
void bind_dataset_properties(nb::class_<T, Ignored...> &c) {
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
void bind_dataset_coord_properties(nb::class_<T, Ignored...> &c) {
  c.def_prop_ro(
      "coords", [](T &self) -> decltype(auto) { return self.coords(); },
      R"(
      Dict of coordinates.)");
}

template <class... Ignored>
void bind_dataset_view_methods(nb::class_<Dataset, Ignored...> &c) {
  bind_common_operators(c);
  c.def("__len__", &Dataset::size);
  c.def(
      "__iter__",
      [](const Dataset &self) {
        return nb::make_iterator<nb::rv_policy::copy>(
            nb::type<Dataset>(), "iterator", self.keys_begin(),
            self.keys_end());
      },
      nb::rv_policy::move, nb::keep_alive<0, 1>());
  c.def(
      "keys", [](Dataset &self) { return keys_view(self); },
      nb::rv_policy::move, nb::keep_alive<0, 1>(),
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
      nb::rv_policy::move, nb::keep_alive<0, 1>(),
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
      nb::rv_policy::move, nb::keep_alive<0, 1>(),
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
      nb::arg("name"),
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
  c.def(
      "__contains__",
      [](const Dataset &self, const nb::handle &key) {
        try {
          return self.contains(nb::cast<std::string>(key));
        } catch (nb::cast_error &) {
          return false; // if `key` is not a string, it cannot be contained
        }
      },
      // The `none` annotation lets None through to the cast_error handler so
      // that `None in self` is False instead of a TypeError.
      nb::arg("key").none());
  c.def("_ipython_key_completions_", [](Dataset &self) {
    nb::typed<nb::list, nb::str> out;
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
void bind_data_array(nb::class_<T, Ignored...> &c) {
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

template <class T> void bind_rebin(nb::module_ &m) {
  m.def(
      "rebin",
      [](const T &x, const std::string &dim, const Variable &bins) {
        return rebin(x, Dim{dim}, bins);
      },
      nb::arg("x"), nb::arg("dim"), nb::arg("bins"),
      nb::call_guard<nb::gil_scoped_release>());
}

template <class Key, class Value> auto to_cpp_dict(const nb::dict &dict) {
  core::Dict<Key, Value> out;
  for (const auto &[key, val] : dict) {
    out.insert_or_assign(Key{nb::cast<std::string>(key)},
                         nb::cast<Value &>(val));
  }
  return out;
}

auto dataset_from_data_and_coords(const nb::dict &data,
                                  const nb::dict &coords) {
  Dataset d;
  for (auto &&[name, item] : data) {
    if (nb::isinstance<DataArray>(item)) {
      d.setDataInit(nb::cast<std::string>(name), nb::cast<DataArray &>(item));
    } else {
      d.setDataInit(nb::cast<std::string>(name), nb::cast<Variable &>(item));
    }
  }
  if (d.is_valid()) {
    // Need to use dataset_from_coords when there is no data to initialize
    // dimensions properly.
    for (auto &&[dim, coord] : coords)
      d.setCoord(Dim{nb::cast<std::string>(dim)}, nb::cast<Variable &>(coord));
  }
  return d;
}

auto dataset_from_coords(const nb::dict &py_coords) {
  typename Coords::holder_type coords;
  for (auto &&[dim, coord] : py_coords)
    coords.insert_or_assign(Dim{nb::cast<std::string>(dim)},
                            nb::cast<Variable &>(coord));
  return Dataset({}, std::move(coords));
}
} // namespace

void init_dataset(nb::module_ &m) {
  static_cast<void>(nb::class_<Slice>(m, "Slice"));

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

  nb::class_<DataArray> dataArray(m, "DataArray", R"(
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
  dataArray.def(
      nb::new_([](const Variable &data, const nb::object &coords,
                  const nb::object &masks, const std::string &name) {
        return DataArray{data, to_cpp_dict<Dim, Variable>(as_pydict(coords)),
                         to_cpp_dict<std::string, Variable>(as_pydict(masks)),
                         name};
      }),
      nb::arg("data"), nb::kw_only(), nb::arg("coords") = nb::dict(),
      nb::arg("masks") = nb::dict(), nb::arg("name") = std::string{},
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
  bind_data_array(dataArray);

  nb::class_<Dataset> dataset(m, "Dataset", R"(
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
  dataset.def(
      nb::new_([](const nb::object &data, const nb::object &coords) {
        if (data.is_none() && coords.is_none())
          throw nb::type_error("Dataset needs data or coordinates or both.");
        const auto data_dict = as_pydict(data);
        const auto coords_dict = as_pydict(coords);
        auto d = dataset_from_data_and_coords(data_dict, coords_dict);
        return d.is_valid() ? d : dataset_from_coords(coords_dict);
      }),
      nb::arg("data") = nb::none(), nb::kw_only(),
      nb::arg("coords") = nb::none(),
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

  dataset
      .def(
          "__setitem__",
          [](Dataset &self, const std::string &name, const Variable &data) {
            self.setData(name, data);
          },
          nb::arg("name"), nb::arg("data"),
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
          nb::arg("name"), nb::arg("data"))
      .def("__delitem__", &Dataset::erase,
           nb::call_guard<nb::gil_scoped_release>())
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
      nb::arg("lhs"), nb::arg("rhs"), nb::call_guard<nb::gil_scoped_release>());

  m.def(
      "irreducible_mask",
      [](const Masks &masks, const std::string &dim) {
        nb::gil_scoped_release release;
        auto mask = irreducible_mask(masks, Dim{dim});
        nb::gil_scoped_acquire acquire;
        return mask.is_valid() ? nb::cast(mask) : nb::none();
      },
      nb::arg("masks"), nb::arg("dim"));

  m.def(
      "reciprocal", [](const DataArray &self) { return reciprocal(self); },
      nb::arg("x"), nb::call_guard<nb::gil_scoped_release>());

  bind_astype(dataArray);

  bind_rebin<DataArray>(m);
  bind_rebin<Dataset>(m);
}
