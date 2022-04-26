// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Hezbrock
#include "scipp/dataset/bins.h"
#include "scipp/core/except.h"
#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

#include "bind_data_array.h"
#include "dim.h"
#include "pybind11.h"

using namespace scipp;

namespace py = pybind11;

namespace {

template <class T>
auto call_make_bins(const std::optional<Variable> &begin_arg,
                    const std::optional<Variable> &end_arg, const Dim dim,
                    T &&data) {
  Variable indices;
  if (begin_arg.has_value()) {
    const auto &begin = *begin_arg;
    if (end_arg.has_value()) {
      const auto &end = *end_arg;
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
  } else if (!end_arg.has_value()) {
    const auto one = scipp::index{1} * units::none;
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
      [](const std::optional<Variable> &begin,
         const std::optional<Variable> &end, const std::string &dim,
         const T &data) {
        return call_make_bins(begin, end, Dim{dim}, T(data));
      },
      py::arg("begin") = py::none(), py::arg("end") = py::none(),
      py::arg("dim"), py::arg("data")); // do not release GIL since using
                                        // implicit conversions in functor
}

template <class T> py::dict bins_constituents(const Variable &var) {
  auto &&[indices, dim, buffer] = var.constituents<T>();
  auto &&[begin, end] = unzip(indices);
  py::dict out;
  out["begin"] = std::forward<decltype(begin)>(begin);
  out["end"] = std::forward<decltype(end)>(end);
  out["dim"] = std::string(dim.name());
  out["data"] = std::forward<decltype(buffer)>(buffer);
  return out;
}

template <class T>
void bind_bins_map_view(py::module &m, const std::string &name) {
  py::class_<T> c(m, name.c_str());
  bind_common_mutable_view_operators<T>(c);
  bind_pop(c);
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

template <class T, class Data>
auto bins_like(const Variable &bins, const Data &data) {
  auto &&[idx, dim, buf] = bins.constituents<T>();
  auto out = make_bins_no_validate(idx, dim, empty_like(data, buf.dims()));
  out.setSlice(Slice{}, data);
  return out;
}

template <class Data> void bind_bins_like(py::module &m) {
  m.def("bins_like", [](const Variable &bins, const Data &data) {
    if (bins.dtype() == dtype<bucket<Variable>>)
      return bins_like<Variable>(bins, data);
    if (bins.dtype() == dtype<bucket<DataArray>>)
      return bins_like<DataArray>(bins, data);
    throw except::TypeError(
        "In `bins_like`: Prototype must contain binned data but got dtype=" +
        to_string(bins.dtype()));
  });
}

} // namespace

void init_buckets(py::module &m) {
  bind_bins<Variable>(m);
  bind_bins<DataArray>(m);
  bind_bins<Dataset>(m);

  bind_bins_like<Variable>(m);

  m.def("is_bins", variable::is_bins);
  m.def("is_bins",
        [](const DataArray &array) { return dataset::is_bins(array); });
  m.def("is_bins",
        [](const Dataset &dataset) { return dataset::is_bins(dataset); });

  m.def("bins_constituents", [](const Variable &var) {
    const auto dt = var.dtype();
    if (dt == dtype<bucket<Variable>>)
      return bins_constituents<Variable>(var);
    if (dt == dtype<bucket<DataArray>>)
      return bins_constituents<DataArray>(var);
    if (dt == dtype<bucket<Dataset>>)
      return bins_constituents<Dataset>(var);
    throw except::TypeError("'constituents' does not support dtype " +
                            to_string(dt));
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
      [](const Variable &var, const std::string &dim) {
        return dataset::buckets::concatenate(var, Dim{dim});
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "concatenate",
      [](const DataArray &array, const std::string &dim) {
        return dataset::buckets::concatenate(array, Dim{dim});
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
  buckets.def(
      "map",
      [](const DataArray &function, const Variable &x, const std::string &dim) {
        return dataset::buckets::map(function, x, Dim{dim});
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "scale",
      [](DataArray &array, const DataArray &histogram, const std::string &dim) {
        return dataset::buckets::scale(array, histogram, Dim{dim});
      },
      py::call_guard<py::gil_scoped_release>());

  m.def(
      "bin",
      [](const DataArray &array, const std::vector<Variable> &edges,
         const std::vector<Variable> &groups,
         const std::vector<std::string> &erase) {
        return dataset::bin(array, edges, groups, to_dim_type(erase));
      },
      py::arg("array"), py::arg("edges"),
      py::arg("groups") = std::vector<Variable>{},
      py::arg("erase") = std::vector<std::string>{},
      py::call_guard<py::gil_scoped_release>());

  bind_bins_view<DataArray>(m);
}
