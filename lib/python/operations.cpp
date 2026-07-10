// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dim.h"
#include "nanobind.h"
#include "slice_utils.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/sort.h"
#include "scipp/variable/math.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/slice.h"
#include "scipp/variable/sort.h"
#include "scipp/variable/util.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace nb = nanobind;

auto get_sort_order(const std::string &order) {
  if (order == "ascending")
    return SortOrder::Ascending;
  else if (order == "descending")
    return SortOrder::Descending;
  else
    throw std::runtime_error("Sort order must be 'ascending' or 'descending'");
}

template <typename T> void bind_dot(nb::module_ &m) {
  m.def(
      "dot", [](const T &x, const T &y) { return dot(x, y); }, nb::arg("x"),
      nb::arg("y"), nb::call_guard<nb::gil_scoped_release>());
}

template <typename T> void bind_sort(nb::module_ &m) {
  m.def(
      "sort",
      [](const T &x, const Variable &key, const std::string &order) {
        return sort(x, key, get_sort_order(order));
      },
      nb::arg("x"), nb::arg("key"), nb::arg("order"),
      nb::call_guard<nb::gil_scoped_release>());
}

template <typename T> void bind_sort_dim(nb::module_ &m) {
  m.def(
      "sort",
      [](const T &x, const std::string &dim, const std::string &order) {
        return sort(x, Dim{dim}, get_sort_order(order));
      },
      nb::arg("x"), nb::arg("key"), nb::arg("order"),
      nb::call_guard<nb::gil_scoped_release>());
}

void bind_issorted(nb::module_ &m) {
  m.def(
      "issorted",
      [](const Variable &x, const std::string &dim, const std::string &order) {
        return issorted(x, Dim{dim}, get_sort_order(order));
      },
      nb::arg("x"), nb::arg("dim"), nb::arg("order") = "ascending",
      nb::call_guard<nb::gil_scoped_release>());
}

void bind_allsorted(nb::module_ &m) {
  m.def(
      "allsorted",
      [](const Variable &x, const std::string &dim, const std::string &order) {
        return allsorted(x, Dim{dim}, get_sort_order(order));
      },
      nb::arg("x"), nb::arg("dim"), nb::arg("order") = "ascending",
      nb::call_guard<nb::gil_scoped_release>());
}

void bind_midpoints(nb::module_ &m) {
  m.def("midpoints", [](const Variable &var,
                        const std::optional<std::string> &dim) {
    return midpoints(var, dim.has_value() ? Dim{*dim} : std::optional<Dim>{});
  });
}

std::tuple<Variable, std::optional<Coords>>
extract_where_argument(const nb::object &arg) {
  if (nb::isinstance<Variable>(arg)) {
    return {nb::cast<Variable>(arg), std::nullopt};
  }
  auto da = nb::cast<DataArray>(arg);
  if (!da.masks().empty()) {
    throw std::invalid_argument("Arguments of 'where' must not have masks");
  }
  return {da.data(), std::optional(da.coords())};
}

std::optional<Coords>
combine_coords_for_where(const std::optional<Coords> &c_coords,
                         std::optional<Coords> &x_coords,
                         const std::optional<Coords> &y_coords) {
  if (x_coords.has_value() && y_coords.has_value()) {
    if (x_coords.value() != y_coords.value()) {
      throw except::CoordMismatchError(
          "Expected coords of x and y to match in 'where' operation");
    }
  }

  if (x_coords.has_value()) {
    if (c_coords.has_value()) {
      return Coords{AutoSizeTag{},
                    union_(c_coords.value(), x_coords.value(), "where")};
    }
    return x_coords.value();
  }
  if (y_coords.has_value()) {
    if (c_coords.has_value()) {
      return Coords{AutoSizeTag{},
                    union_(c_coords.value(), y_coords.value(), "where")};
    }
    return y_coords.value();
  }
  return std::nullopt;
}

void bind_where(nb::module_ &m) {
  m.def(
      "where",
      [](const nb::object &condition, const nb::object &x,
         const nb::object &y) {
        auto [c_data, c_coords] = extract_where_argument(condition);
        auto [x_data, x_coords] = extract_where_argument(x);
        auto [y_data, y_coords] = extract_where_argument(y);
        auto coords = combine_coords_for_where(c_coords, x_coords, y_coords);
        Variable new_data = where(c_data, x_data, y_data);

        if (coords.has_value()) {
          return nb::cast(
              DataArray(std::move(new_data), std::move(coords.value()), {}));
        }
        return nb::cast(std::move(new_data));
      },
      nb::arg("condition"), nb::arg("x"), nb::arg("y"));
}

void init_operations(nb::module_ &m) {
  bind_dot<Variable>(m);

  bind_sort<Variable>(m);
  bind_sort<DataArray>(m);
  bind_sort<Dataset>(m);
  bind_sort_dim<Variable>(m);
  bind_sort_dim<DataArray>(m);
  bind_sort_dim<Dataset>(m);
  bind_issorted(m);
  bind_allsorted(m);
  bind_midpoints(m);
  bind_where(m);

  m.def(
      "label_based_index_to_positional_index",
      [](const std::vector<std::string> &dims,
         const std::vector<scipp::index> &shape, const Variable &coord,
         const Variable &value) {
        const auto [dim, index] =
            get_slice_params(make_dims(dims, shape), coord, value);
        return std::tuple{dim.name(), index};
      },
      nb::call_guard<nb::gil_scoped_release>());
  m.def("label_based_index_to_positional_index",
        [](const std::vector<std::string> &dims,
           const std::vector<scipp::index> &shape, const Variable &coord,
           const nb::slice &py_slice) {
          try {
            auto [start_var, stop_var] = label_bounds_from_pyslice(py_slice);
            const auto [dim, start, stop] = get_slice_params(
                make_dims(dims, shape), coord, start_var, stop_var);
            return std::tuple{dim.name(), start, stop};
          } catch (const nb::cast_error &) {
            throw std::runtime_error(
                "Value based slice must contain variables.");
          }
        });
}
