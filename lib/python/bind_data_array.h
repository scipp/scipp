// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <pybind11/typing.h>

#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable_factory.h"

#include "bind_operators.h"
#include "pybind11.h"
#include "view.h"

namespace py = pybind11;
using namespace scipp;

template <template <class> class View, class T>
void bind_helper_view(py::module &m, const std::string &name) {
  std::string suffix;
  if constexpr (std::is_same_v<View<T>, items_view<T>> ||
                std::is_same_v<View<T>, str_items_view<T>>)
    suffix = "_items_view";
  if constexpr (std::is_same_v<View<T>, values_view<T>>)
    suffix = "_values_view";
  if constexpr (std::is_same_v<View<T>, keys_view<T>> ||
                std::is_same_v<View<T>, str_keys_view<T>>)
    suffix = "_keys_view";
  auto cls =
      py::class_<View<T>>(m, (name + suffix).c_str())
          .def("__len__", &View<T>::size)
          .def("__repr__", [](const View<T> &self) { return self.tostring(); })
          .def("__str__", [](const View<T> &self) { return self.tostring(); })
          .def(
              "__iter__",
              [](const View<T> &self) {
                return py::make_iterator(self.begin(), self.end(),
                                         py::return_value_policy::move);
              },
              py::return_value_policy::move, py::keep_alive<0, 1>());
  if constexpr (!std::is_same_v<View<T>, values_view<T>>)
    cls.def("__eq__", [](const View<T> &self, const View<T> &other) {
      return self == other;
    });
}

template <class D> auto cast_to_dict_key(const py::handle &obj) {
  using key_type = typename D::key_type;
  if constexpr (std::is_same_v<key_type, std::string>) {
    return obj.cast<std::string>();
  } else {
    return key_type{obj.cast<std::string>()};
  }
}

template <class D> auto cast_to_dict_value(const py::handle &obj) {
  using val_type = typename D::mapped_type;
  return obj.cast<val_type>();
}

template <class T, class... Ignored>
void bind_common_mutable_view_operators(pybind11::class_<T, Ignored...> &view) {
  view.def("__len__", &T::size)
      .def(
          "__getitem__",
          [](const T &self, const std::string &key) {
            return self[typename T::key_type{key}];
          },
          py::return_value_policy::copy)
      .def("__setitem__",
           [](T &self, const std::string &key, const Variable &var) {
             self.set(typename T::key_type{key}, var);
           })
      .def(
          "__delitem__",
          [](T &self, const std::string &key) {
            self.erase(typename T::key_type{key});
          },
          py::call_guard<py::gil_scoped_release>())
      .def("__contains__", [](const T &self, const py::handle &key) {
        try {
          return self.contains(cast_to_dict_key<T>(key));
        } catch (py::cast_error &) {
          return false; // if `key` is not a string, it cannot be contained
        }
      });
}

template <class T, class... Ignored, class Set>
void bind_dict_update(pybind11::class_<T, Ignored...> &view, Set &&set_item) {
  view.def(
      "update",
      [set_item](T &self, const py::object &other, const py::kwargs &kwargs) {
        // Piggyback on dict to implement argument handling.
        py::dict args;
        if (!other.is_none()) {
          args.attr("update")(other, **kwargs);
        } else {
          // Cannot call dict.update(None, **kwargs) because dict.update
          // expects either an iterable as the positional argument or nothing
          // at all (nullptr). But we cannot express 'nothing' here.
          // The best we can do it pass None, which does not work.
          args.attr("update")(**kwargs);
        }

        for (const auto &[key, val] : args) {
          set_item(self, cast_to_dict_key<T>(key), cast_to_dict_value<T>(val));
        }
      },
      py::arg("other") = py::none(), py::pos_only(),
      R"doc(Update items from dict-like or iterable.

If ``other`` has a .keys() method, then update does:
``for k in other.keys(): self[k] = other[k]``.

If ``other`` is given but does not have a .keys() method, then update does:
``for k, v in other: self[k] = v``.

In either case, this is followed by:
``for k in kwargs: self[k] = kwargs[k]``.

See Also
--------
dict.update
)doc");
}

template <class T, class... Ignored>
void bind_pop(pybind11::class_<T, Ignored...> &view) {
  view.def(
      "_pop",
      [](T &self, const std::string &key) {
        return self.extract(typename T::key_type{key});
      },
      py::arg("k"));
}

template <class T, class... Ignored>
void bind_set_aligned(pybind11::class_<T, Ignored...> &view) {
  view.def(
      "set_aligned",
      // cppcheck-suppress constParameter  # False positive.
      [](T &self, const std::string &key, const bool aligned) {
        self.set_aligned(typename T::key_type{key}, aligned);
      },
      py::arg("key"), py::arg("aligned"),
      R"(Set the alignment flag for a coordinate.

Aligned coordinates (the default) are compared in binary operations and
must match. Unaligned coordinates are not compared and are dropped if
they do not match.

Parameters
----------
key:
    Name of the coordinate.
aligned:
    True to mark as aligned, False to mark as unaligned.

Examples
--------

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
  ...     coords={'x': sc.arange('x', 3, unit='m')}
  ... )

Mark a coordinate as unaligned:

  >>> da.coords.set_aligned('x', False)

Unaligned coordinates are shown without the '*' prefix in the repr.
)");
}

template <class T, class... Ignored>
void bind_dict_clear(pybind11::class_<T, Ignored...> &view) {
  view.def("clear", [](T &self) {
    std::vector<typename T::key_type> keys;
    for (const auto &key : keys_view(self))
      keys.push_back(key);
    for (const auto &key : keys)
      self.erase(key);
  });
}

template <class T, class... Ignored>
void bind_dict_popitem(pybind11::class_<T, Ignored...> &view) {
  view.def("popitem", [](T &self) {
    typename T::key_type key;
    for (const auto &k : keys_view(self))
      key = k;
    const auto item = py::cast(self.extract(key));

    using Pair =
        py::typing::Tuple<py::str, std::decay_t<decltype(self.extract(key))>>;
    Pair result(2);
    if constexpr (std::is_same_v<typename T::key_type, Dim>)
      result[0] = key.name();
    else
      result[0] = key;
    result[1] = item;
    return result;
  });
}

template <class T, class... Ignored>
void bind_dict_copy(pybind11::class_<T, Ignored...> &view) {
  view.def(
          "copy",
          [](const T &self, const bool deep) {
            return deep ? copy(self) : self;
          },
          py::arg("deep") = true, py::call_guard<py::gil_scoped_release>(),
          R"(
      Return a (by default deep) copy.

      If `deep=True` (the default), a deep copy is made. Otherwise, a shallow
      copy is made, and the returned data (and meta data) values are new views
      of the data and meta data values of this object.)")
      .def(
          "__copy__", [](const T &self) { return self; },
          py::call_guard<py::gil_scoped_release>(), "Return a (shallow) copy.")
      .def(
          "__deepcopy__",
          [](const T &self, const py::typing::Dict<py::object, py::object> &) {
            return copy(self);
          },
          py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
}

template <class T, class... Ignored>
void bind_is_edges(py::class_<T, Ignored...> &view) {
  view.def(
      "is_edges",
      [](const T &self, const std::string &key,
         const std::optional<std::string> &dim) {
        return self.is_edges(typename T::key_type{key},
                             dim.has_value() ? std::optional{Dim(*dim)}
                                             : std::optional<Dim>{});
      },
      py::arg("key"), py::arg("dim") = std::nullopt,
      R"(Return True if the given key contains bin-edges in the given dim.

Bin-edge coordinates have one more element than the corresponding dimension
size. They define the boundaries of histogram bins.

Parameters
----------
key:
    Name of the coordinate to check.
dim:
    Dimension to check against. If not provided, checks the coordinate's
    single dimension.

Returns
-------
:
    True if the coordinate is a bin-edge coordinate.

Examples
--------

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
  ...     coords={'x': sc.array(dims=['x'], values=[0.0, 1.0, 2.0, 3.0])}
  ... )
  >>> da.coords.is_edges('x')
  True

Point coordinates have the same size as the dimension:

  >>> da2 = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
  ...     coords={'x': sc.array(dims=['x'], values=[0.5, 1.5, 2.5])}
  ... )
  >>> da2.coords.is_edges('x')
  False
)");
}

template <class T>
void bind_mutable_view(py::module &m, const std::string &name,
                       const std::string &docs) {
  py::class_<T> view(m, name.c_str(), docs.c_str());
  bind_common_mutable_view_operators(view);
  bind_inequality_to_operator<T>(view);
  bind_dict_update(view, [](T &self, const std::string &key,
                            const Variable &value) { self.set(key, value); });
  bind_pop(view);
  bind_dict_clear(view);
  bind_dict_popitem(view);
  bind_dict_copy(view);
  bind_is_edges(view);
  view.def(
          "__iter__",
          [](const T &self) {
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
      .def("_ipython_key_completions_",
           [](const T &self) {
             py::typing::List<py::str> out;
             const auto end = self.keys_end();
             for (auto it = self.keys_begin(); it != end; ++it) {
               out.append(*it);
             }
             return out;
           })
      .def("__repr__", [name](const T &self) { return to_string(self); })
      .def("__str__", [name](const T &self) { return to_string(self); });
}

template <class T>
void bind_mutable_view_no_dim(py::module &m, const std::string &name,
                              const std::string &docs) {
  py::class_<T> view(m, name.c_str(), docs.c_str());
  bind_common_mutable_view_operators(view);
  bind_inequality_to_operator<T>(view);
  bind_dict_update(view, [](T &self, const sc_units::Dim &key,
                            const Variable &value) { self.set(key, value); });
  bind_pop(view);
  bind_set_aligned(view);
  bind_dict_clear(view);
  bind_dict_popitem(view);
  bind_dict_copy(view);
  bind_is_edges(view);
  view.def(
          "__iter__",
          [](T &self) {
            auto keys_view = str_keys_view(self);
            return py::make_iterator(keys_view.begin(), keys_view.end(),
                                     py::return_value_policy::move);
          },
          py::keep_alive<0, 1>())
      .def(
          "keys", [](T &self) { return str_keys_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's keys)")
      .def(
          "values", [](T &self) { return values_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's values)")
      .def(
          "items", [](T &self) { return str_items_view(self); },
          py::return_value_policy::move, py::keep_alive<0, 1>(),
          R"(view on self's items)")
      .def("_ipython_key_completions_",
           [](const T &self) {
             py::typing::List<py::str> out;
             const auto end = self.keys_end();
             for (auto it = self.keys_begin(); it != end; ++it) {
               out.append(it->name());
             }
             return out;
           })
      .def("__repr__", [name](const T &self) { return to_string(self); })
      .def("__str__", [name](const T &self) { return to_string(self); });
}

template <class T, class... Ignored>
void bind_data_array_properties(py::class_<T, Ignored...> &c) {
  if constexpr (std::is_same_v<T, DataArray>)
    c.def_property("name", &T::name, &T::setName,
                   R"(The name of the held data.

Examples
--------

  >>> import scipp as sc
  >>> da = sc.DataArray(sc.array(dims=['x'], values=[1.0, 2.0, 3.0]))
  >>> da.name
  ''
  >>> da.name = 'temperature'
  >>> da.name
  'temperature'

The name is preserved through operations:

  >>> summed = da.sum()
  >>> summed.name
  'temperature'
)");
  else
    c.def_property_readonly("name", &T::name, R"(The name of the held data.)");
  c.def_property(
      "data",
      py::cpp_function([](T &self) { return self.data(); },
                       py::return_value_policy::copy),
      [](T &self, const Variable &data) { self.setData(data); },
      R"(Underlying data Variable.

The data property provides access to the data values of a DataArray as a
Variable, without the coordinates and masks.

Examples
--------

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m'),
  ...     coords={'x': sc.arange('x', 3, unit='s')}
  ... )
  >>> da.data
  <scipp.Variable> (x: 3)    float64              [m]  [1, 2, 3]

The data can be replaced entirely:

  >>> da.data = sc.array(dims=['x'], values=[10.0, 20.0, 30.0], unit='m')
  >>> da.data
  <scipp.Variable> (x: 3)    float64              [m]  [10, 20, 30]
)");
  c.def_property_readonly(
      "coords", [](T &self) -> decltype(auto) { return self.coords(); },
      R"(Dict of coordinates.

Coordinates define the axis labels for each dimension. They can be
point-coordinates (one value per data point) or bin-edge coordinates
(one more value than data points).

Examples
--------

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x', 'y'], values=[[1, 2], [3, 4]]),
  ...     coords={
  ...         'x': sc.array(dims=['x'], values=[0.0, 1.0], unit='m'),
  ...         'y': sc.array(dims=['y'], values=[10.0, 20.0], unit='s')
  ...     }
  ... )
  >>> da.coords
  <scipp.Dict>
    x: <scipp.Variable> (x: 2)    float64              [m]  [0, 1]
    y: <scipp.Variable> (y: 2)    float64              [s]  [10, 20]

Access individual coordinates:

  >>> da.coords['x']
  <scipp.Variable> (x: 2)    float64              [m]  [0, 1]

List coordinate names:

  >>> da.coords.keys()
  <scipp.Dict.keys {x, y}>
)");
  c.def_property_readonly(
      "masks", [](T &self) -> decltype(auto) { return self.masks(); },
      R"(Dict of masks.

Masks are boolean Variables that mark data points as valid (False) or
invalid (True). Masked data is excluded from most operations.

Examples
--------

  >>> import scipp as sc
  >>> da = sc.DataArray(sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0]))
  >>> da.masks['outliers'] = sc.array(dims=['x'], values=[False, False, True, False])
  >>> da.masks
  <scipp.Dict>
    outliers: <scipp.Variable> (x: 4)       bool        <no unit>  [False, False, True, False]

Check if a mask exists:

  >>> 'outliers' in da.masks
  True

Access a mask:

  >>> da.masks['outliers']
  <scipp.Variable> (x: 4)       bool        <no unit>  [False, False, True, False]

Masked values are excluded from reductions:

  >>> float(da.sum().value)  # third element (3.0) is masked out
  7.0
)");
  c.def(
      "drop_coords",
      [](T &self, const std::string &coord_name) {
        std::vector<scipp::Dim> coord_names_c = {scipp::Dim{coord_name}};
        return self.drop_coords(coord_names_c);
      },
      py::arg("coord_names"),
      R"(Return new object with specified coordinate(s) removed.

Parameters
----------
coord_names:
    Name of the coordinate to remove, or a list of names.

Returns
-------
:
    New DataArray without the specified coordinate(s).

Examples
--------
Remove a single coordinate:

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
  ...     coords={
  ...         'x': sc.arange('x', 3),
  ...         'y': sc.array(dims=['x'], values=[10, 20, 30])
  ...     }
  ... )
  >>> da.drop_coords('y')
  <scipp.DataArray>
  Dimensions: Sizes[x:3, ]
  Coordinates:
  * x                           int64  [dimensionless]  (x)  [0, 1, 2]
  Data:
                              float64  [dimensionless]  (x)  [1, 2, 3]

Remove multiple coordinates:

  >>> da.coords['z'] = sc.array(dims=['x'], values=[100, 200, 300])
  >>> da.drop_coords(['y', 'z'])
  <scipp.DataArray>
  Dimensions: Sizes[x:3, ]
  Coordinates:
  * x                           int64  [dimensionless]  (x)  [0, 1, 2]
  Data:
                              float64  [dimensionless]  (x)  [1, 2, 3]
)");
  c.def(
      "drop_coords",
      [](T &self, const std::vector<std::string> &coord_names) {
        std::vector<scipp::Dim> coord_names_c;
        std::transform(coord_names.begin(), coord_names.end(),
                       std::back_inserter(coord_names_c),
                       [](const auto &name) { return scipp::Dim{name}; });
        return self.drop_coords(coord_names_c);
      },
      py::arg("coord_names"));
  c.def(
      "drop_masks",
      [](T &self, const std::string &mask_name) {
        return self.drop_masks(std::vector({mask_name}));
      },
      py::arg("mask_names"),
      R"(Return new object with specified mask(s) removed.

Parameters
----------
mask_names:
    Name of the mask to remove, or a list of names.

Returns
-------
:
    New DataArray without the specified mask(s).

Examples
--------
Remove a single mask:

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
  ...     coords={'x': sc.arange('x', 3)},
  ...     masks={
  ...         'm1': sc.array(dims=['x'], values=[False, True, False]),
  ...         'm2': sc.array(dims=['x'], values=[True, False, False])
  ...     }
  ... )
  >>> da.drop_masks('m1')
  <scipp.DataArray>
  Dimensions: Sizes[x:3, ]
  Coordinates:
  * x                           int64  [dimensionless]  (x)  [0, 1, 2]
  Data:
                              float64  [dimensionless]  (x)  [1, 2, 3]
  Masks:
    m2                           bool        <no unit>  (x)  [True, False, False]

Remove multiple masks:

  >>> da.drop_masks(['m1', 'm2'])
  <scipp.DataArray>
  Dimensions: Sizes[x:3, ]
  Coordinates:
  * x                           int64  [dimensionless]  (x)  [0, 1, 2]
  Data:
                              float64  [dimensionless]  (x)  [1, 2, 3]
)");
  c.def(
      "drop_masks",
      [](T &self, std::vector<std::string> &mask_names) {
        return self.drop_masks(mask_names);
      },
      py::arg("mask_names"));
}
