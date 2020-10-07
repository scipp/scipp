// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Hezbrock
#include "pybind11.h"
#include "scipp/core/except.h"
#include "scipp/dataset/bucket.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.h"

using namespace scipp;

namespace py = pybind11;

namespace {

template <class T> void bind_buckets(pybind11::module &m) {
  m.def(
      "to_buckets",
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
        return Variable(std::make_unique<variable::DataModel<bucket<T>>>(
            makeVariable<std::pair<scipp::index, scipp::index>>(
                dims, Values(std::move(indices))),
            dim, T(data)));
      },
      py::arg("begin") = py::none(), py::arg("end") = py::none(),
      py::arg("dim"), py::arg("data")); // do not release GIL since using
                                        // implicit conversions in functor
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
  bind_buckets<Variable>(m);
  bind_buckets<DataArray>(m);
  bind_buckets<Dataset>(m);

  auto buckets = m.def_submodule("buckets");
  buckets.def("concatenate", dataset::buckets::concatenate,
              py::call_guard<py::gil_scoped_release>());
  buckets.def("append", dataset::buckets::append,
              py::call_guard<py::gil_scoped_release>());
  buckets.def("histogram", dataset::buckets::histogram,
              py::call_guard<py::gil_scoped_release>());
  buckets.def("map", dataset::buckets::map,
              py::call_guard<py::gil_scoped_release>());
  buckets.def("scale", dataset::buckets::scale,
              py::call_guard<py::gil_scoped_release>());
  buckets.def("get_buffer", [](py::object &obj) -> py::object {
    auto &var = obj.cast<const VariableView &>();
    if (var.dtype() == dtype<bucket<Variable>>)
      return get_buffer<Variable>(obj);
    if (var.dtype() == dtype<bucket<DataArray>>)
      return get_buffer<DataArray>(obj);
    if (var.dtype() == dtype<bucket<Dataset>>)
      return get_buffer<Dataset>(obj);
    return py::none();
  });
}
