// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Hezbrock
#include "pybind11.h"
#include "scipp/core/except.h"
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
      [](const VariableConstView &begin, const VariableConstView &end,
         const Dim dim, const typename T::const_view_type &data) {
        element_array<std::pair<scipp::index, scipp::index>> buckets;
        Dimensions dims;
        if (begin) {
          buckets.resize(begin.dims().volume());
          dims = begin.dims();
          const auto &begin_ = begin.values<int64_t>();
          if (end) {
            core::expect::equals(begin.dims(), end.dims());
            const auto &end_ = end.values<int64_t>();
            for (scipp::index i = 0; i < buckets.size(); ++i)
              buckets.data()[i] = {begin_[i], end_[i]};
          } else {
            for (scipp::index i = 0; i < buckets.size(); ++i)
              buckets.data()[i] = {begin_[i], -1};
          }
        } else if (!end) {
          buckets.resize(data.dims()[dim]);
          dims = Dimensions(dim, buckets.size());
          for (scipp::index i = 0; i < buckets.size(); ++i)
            buckets.data()[i] = {i, -1};
        } else {
          throw std::runtime_error("`end` given but not `begin`");
        }
        return Variable(std::make_unique<variable::DataModel<bucket<T>>>(
            dims, std::move(buckets), dim, T(data)));
      },
      py::arg("begin") = VariableConstView{},
      py::arg("end") = VariableConstView{}, py::arg("dim"), py::arg("data"),
      py::call_guard<py::gil_scoped_release>());
}

} // namespace

void init_buckets(py::module &m) {
  bind_buckets<Variable>(m);
  bind_buckets<DataArray>(m);
  bind_buckets<Dataset>(m);
}
