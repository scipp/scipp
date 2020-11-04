// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Hezbrock
#include "pybind11.h"
#include "scipp/core/except.h"
#include "scipp/dataset/bucket.h"
#include "scipp/dataset/bucketby.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

using namespace scipp;

namespace py = pybind11;

namespace {

template <class T> void bind_bins(pybind11::module &m) {
  m.def(
      "bins",
      [](const py::object &begin_obj, const py::object &end_obj, const Dim dim,
         const typename T::const_view_type &data) {
        element_array<std::pair<scipp::index, scipp::index>> indices;
        Dimensions dims;
        if (!begin_obj.is_none()) {
          const auto &begin = begin_obj.cast<VariableView>();
          indices.resize(begin.dims().volume());
          dims = begin.dims();
          const auto &begin_ = begin.values<int64_t>();
          if (!end_obj.is_none()) {
            const auto &end = end_obj.cast<VariableView>();
            core::expect::equals(begin.dims(), end.dims());
            const auto &end_ = end.values<int64_t>();
            for (scipp::index i = 0; i < indices.size(); ++i)
              indices.data()[i] = {begin_[i], end_[i]};
          } else {
            for (scipp::index i = 0; i < indices.size(); ++i)
              indices.data()[i] = {begin_[i], -1};
          }
        } else if (end_obj.is_none()) {
          indices.resize(data.dims()[dim]);
          dims = Dimensions(dim, indices.size());
          for (scipp::index i = 0; i < indices.size(); ++i)
            indices.data()[i] = {i, -1};
        } else {
          throw std::runtime_error("`end` given but not `begin`");
        }
        return from_constituents(
            makeVariable<std::pair<scipp::index, scipp::index>>(
                dims, Values(std::move(indices))),
            dim, T(data));
      },
      py::arg("begin") = py::none(), py::arg("end") = py::none(),
      py::arg("dim"), py::arg("data")); // do not release GIL since using
                                        // implicit conversions in functor
}

template <class T> void bind_bin_size(pybind11::module &m) {
  m.def(
      "bin_size",
      [](const typename T::const_view_type &x) {
        return dataset::bucket_sizes(x);
      },
      py::call_guard<py::gil_scoped_release>());
}

template <class T> auto bin_begin_end(const VariableConstView &var) {
  auto &&[indices, dim, buffer] = var.constituents<bucket<T>>();
  return py::cast(unzip(indices));
}

template <class T> auto bin_dim(const VariableConstView &var) {
  auto &&[indices, dim, buffer] = var.constituents<bucket<T>>();
  return py::cast(dim);
}

template <class T> auto get_buffer(py::object &obj) {
  auto &view = obj.cast<const VariableView &>();
  auto &&[indices, dim, buffer] = view.constituents<bucket<T>>();
  auto ret =
      py::cast(typename T::view_type(buffer), py::return_value_policy::move);
  pybind11::detail::keep_alive_impl(ret, obj);
  return ret;
}

} // namespace

void init_buckets(py::module &m) {
  bind_bins<Variable>(m);
  bind_bins<DataArray>(m);
  bind_bins<Dataset>(m);

  bind_bin_size<Variable>(m);
  bind_bin_size<DataArray>(m);
  bind_bin_size<Dataset>(m);

  m.def("is_bins", variable::is_buckets);
  m.def("is_bins", [](const DataArrayConstView &array) {
    return dataset::is_buckets(array);
  });
  m.def("is_bins", [](const DatasetConstView &dataset) {
    return dataset::is_buckets(dataset);
  });

  m.def("bins_begin_end", [](const VariableConstView &var) -> py::object {
    if (var.dtype() == dtype<bucket<Variable>>)
      return bin_begin_end<Variable>(var);
    if (var.dtype() == dtype<bucket<DataArray>>)
      return bin_begin_end<DataArray>(var);
    if (var.dtype() == dtype<bucket<Dataset>>)
      return bin_begin_end<Dataset>(var);
    return py::none();
  });

  m.def("bins_dim", [](const VariableConstView &var) -> py::object {
    if (var.dtype() == dtype<bucket<Variable>>)
      return bin_dim<Variable>(var);
    if (var.dtype() == dtype<bucket<DataArray>>)
      return bin_dim<DataArray>(var);
    if (var.dtype() == dtype<bucket<Dataset>>)
      return bin_dim<Dataset>(var);
    return py::none();
  });

  m.def("bins_data", [](py::object &obj) -> py::object {
    auto &var = obj.cast<const VariableView &>();
    if (var.dtype() == dtype<bucket<Variable>>)
      return get_buffer<Variable>(obj);
    if (var.dtype() == dtype<bucket<DataArray>>)
      return get_buffer<DataArray>(obj);
    if (var.dtype() == dtype<bucket<Dataset>>)
      return get_buffer<Dataset>(obj);
    return py::none();
  });

  auto buckets = m.def_submodule("buckets");
  buckets.def(
      "concatenate",
      [](const VariableConstView &a, const VariableConstView &b) {
        return dataset::buckets::concatenate(a, b);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "concatenate",
      [](const DataArrayConstView &a, const DataArrayConstView &b) {
        return dataset::buckets::concatenate(a, b);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "concatenate",
      [](const VariableConstView &var, const Dim dim) {
        return dataset::buckets::concatenate(var, dim);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "concatenate",
      [](const DataArrayConstView &array, const Dim dim) {
        return dataset::buckets::concatenate(array, dim);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "append",
      [](const VariableView &a, const VariableConstView &b) {
        return dataset::buckets::append(a, b);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "append",
      [](const DataArrayView &a, const DataArrayConstView &b) {
        return dataset::buckets::append(a, b);
      },
      py::call_guard<py::gil_scoped_release>());
  buckets.def("map", dataset::buckets::map,
              py::call_guard<py::gil_scoped_release>());
  buckets.def("scale", dataset::buckets::scale,
              py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "sum",
      [](const VariableConstView &x) { return dataset::buckets::sum(x); },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "sum",
      [](const DataArrayConstView &x) { return dataset::buckets::sum(x); },
      py::call_guard<py::gil_scoped_release>());
  buckets.def(
      "sum", [](const DatasetConstView &x) { return dataset::buckets::sum(x); },
      py::call_guard<py::gil_scoped_release>());

  m.def("bucketby", dataset::bucketby,
        py::call_guard<py::gil_scoped_release>());
}
