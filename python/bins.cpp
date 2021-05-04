// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Hezbrock
#include "scipp/dataset/bins.h"
#include "scipp/core/except.h"
#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/rebin.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

#include "bind_data_array.h"
#include "pybind11.h"

using namespace scipp;

namespace py = pybind11;

namespace {

template <class T>
auto call_make_bins(const py::object &begin_obj, const py::object &end_obj,
                    const Dim dim, T &&data) {
  Variable indices;
  Dimensions dims;
  if (!begin_obj.is_none()) {
    const auto &begin = begin_obj.cast<Variable>();
    dims = begin.dims();
    if (!end_obj.is_none()) {
      const auto &end = end_obj.cast<Variable>();
      indices = zip(begin, end);
    } else {
      indices = zip(begin, begin);
      const auto indices_ = indices.values<scipp::index_pair>();
      const auto nindex = scipp::size(indices_);
      for (scipp::index i = 0; i < nindex; ++i) {
        if (i < nindex - 1)
          indices_[i].second = indices_[i + 1].first;
        else
          indices_[i].second = data.dims()[dim];
      }
    }
  } else if (end_obj.is_none()) {
    const auto one = scipp::index{1} * units::one;
    const auto ones = broadcast(one, {dim, data.dims()[dim]});
    const auto begin = cumsum(ones, dim, CumSumMode::Exclusive);
    indices = zip(begin, begin + one);
  } else {
    throw std::runtime_error("`end` given but not `begin`");
  }
  return make_bins(std::move(indices), dim, std::forward<T>(data));
}

template <class T> void bind_bins(pybind11::module &m) {
  m.def(
      "bins",
      [](const py::object &begin_obj, const py::object &end_obj, const Dim dim,
         const T &data) {
        return call_make_bins(begin_obj, end_obj, dim, T(data));
      },
      py::arg("begin") = py::none(), py::arg("end") = py::none(),
      py::arg("dim"), py::arg("data")); // do not release GIL since using
                                        // implicit conversions in functor
}

template <class T> void bind_bin_size(pybind11::module &m) {
  m.def(
      "bin_size", [](const T &x) { return dataset::bucket_sizes(x); },
      py::call_guard<py::gil_scoped_release>());
}

template <class T> auto bin_begin_end(const Variable &var) {
  auto &&[indices, dim, buffer] = var.constituents<bucket<T>>();
  static_cast<void>(dim);
  static_cast<void>(buffer);
  return py::cast(unzip(indices));
}

template <class T> auto bin_dim(const Variable &var) {
  auto &&[indices, dim, buffer] = var.constituents<bucket<T>>();
  static_cast<void>(buffer);
  static_cast<void>(indices);
  return py::cast(std::string(dim.name()));
}

template <class T>
void bind_bins_map_view(py::module &m, const std::string &name) {
  py::class_<T> c(m, name.c_str());
  bind_common_mutable_view_operators<T>(c);
}

template <class T> void bind_bins_view(py::module &m) {
  py::class_<decltype(dataset::bins_view<T>(Variable{}))> c(
      m, "_BinsViewDataArray");
  bind_bins_map_view<decltype(dataset::bins_view<T>(Variable{}).meta())>(
      m, "_BinsMeta");
  bind_bins_map_view<decltype(dataset::bins_view<T>(Variable{}).coords())>(
      m, "_BinsCoords");
  bind_bins_map_view<decltype(dataset::bins_view<T>(Variable{}).masks())>(
      m, "_BinsMasks");
  bind_bins_map_view<decltype(dataset::bins_view<T>(Variable{}).attrs())>(
      m, "_BinsAttrs");
  bind_data_array_properties(c);
  m.def("_bins_view", [](Variable &var) { return dataset::bins_view<T>(var); });
}

} // namespace

void init_buckets(py::module &m) {
  bind_bins<Variable>(m);
  bind_bins<DataArray>(m);
  bind_bins<Dataset>(m);

  bind_bin_size<Variable>(m);
  bind_bin_size<DataArray>(m);
  bind_bin_size<Dataset>(m);

  m.def("is_bins", variable::is_bins);
  m.def("is_bins",
        [](const DataArray &array) { return dataset::is_bins(array); });
  m.def("is_bins",
        [](const Dataset &dataset) { return dataset::is_bins(dataset); });

  m.def("bins_begin_end", [](const Variable &var) -> py::object {
    if (var.dtype() == dtype<bucket<Variable>>)
      return bin_begin_end<Variable>(var);
    if (var.dtype() == dtype<bucket<DataArray>>)
      return bin_begin_end<DataArray>(var);
    if (var.dtype() == dtype<bucket<Dataset>>)
      return bin_begin_end<Dataset>(var);
    return py::none();
  });

  m.def("bins_dim", [](const Variable &var) -> py::object {
    if (var.dtype() == dtype<bucket<Variable>>)
      return bin_dim<Variable>(var);
    if (var.dtype() == dtype<bucket<DataArray>>)
      return bin_dim<DataArray>(var);
    if (var.dtype() == dtype<bucket<Dataset>>)
      return bin_dim<Dataset>(var);
    return py::none();
  });

  m.def("bins_data", [](py::object &obj) -> py::object {
    auto &var = obj.cast<Variable &>();
    if (var.dtype() == dtype<bucket<Variable>>)
      return py::cast(obj.cast<Variable &>().bin_buffer<Variable>());
    if (var.dtype() == dtype<bucket<DataArray>>)
      return py::cast(obj.cast<Variable &>().bin_buffer<DataArray>().view());
    if (var.dtype() == dtype<bucket<Dataset>>)
      // TODO Provide mechanism for creating sharing view as for DataArray above
      return py::cast(obj.cast<Variable &>().bin_buffer<Dataset>());
    return py::none();
  });

  auto buckets = m.def_submodule("buckets");
  buckets.def(
      "concatenate",
      [](const Variable &a, const Variable &b) {
        return dataset::buckets::concatenate(a, b);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "concatenate",
      [](const DataArray &a, const DataArray &b) {
        return dataset::buckets::concatenate(a, b);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "concatenate",
      [](const Variable &var, const Dim dim) {
        return dataset::buckets::concatenate(var, dim);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "concatenate",
      [](const DataArray &array, const Dim dim) {
        return dataset::buckets::concatenate(array, dim);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "append",
      [](Variable &a, const Variable &b) {
        return dataset::buckets::append(a, b);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "append",
      [](DataArray &a, const DataArray &b) {
        return dataset::buckets::append(a, b);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def("map", dataset::buckets::map,
              py::call_guard<py::gil_scoped_release>());
  buckets.def("scale", dataset::buckets::scale,
              py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "sum", [](const Variable &x) { return dataset::buckets::sum(x); },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "sum", [](const DataArray &x) { return dataset::buckets::sum(x); },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "sum", [](const Dataset &x) { return dataset::buckets::sum(x); },
      py::call_guard<py::gil_scoped_release>());

  m.def(
      "bin",
      [](const DataArray &array, const std::vector<Variable> &edges,
         const std::vector<Variable> &groups, const std::vector<Dim> &erase) {
        return dataset::bin(array, edges, groups, erase);
      },
      py::call_guard<py::gil_scoped_release>());
  m.def("bin_with_coords", [](const Variable &data, const py::dict &coords,
                              const std::vector<Variable> &edges,
                              const std::vector<Variable> &groups) {
    std::map<Dim, Variable> c;
    for (const auto [name, coord] : coords)
      c.emplace(Dim(py::cast<std::string>(name)), py::cast<Variable>(coord));
    py::gil_scoped_release release; // release only *after* using py::cast
    return dataset::bin(data, c, std::map<std::string, Variable>{},
                        std::map<Dim, Variable>{}, edges, groups);
  });

  bind_bins_view<DataArray>(m);
}
