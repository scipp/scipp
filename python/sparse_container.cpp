// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/variable.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(sparse_container<double>)
PYBIND11_MAKE_OPAQUE(sparse_container<float>)
PYBIND11_MAKE_OPAQUE(sparse_container<int64_t>)

template <class T>
void declare_sparse_container(py::module &m, const std::string &suffix) {
  auto s = py::bind_vector<sparse_container<T>>(
      m, std::string("sparse_container_") + suffix, py::buffer_protocol());
  // pybind11 currently does not find method from the base class, see
  // https://github.com/pybind/pybind11/pull/1832. We add the method manually
  // here.
  s.def("__len__", [](const sparse_container<T> &self) { return self.size(); });
}

void init_sparse_container(py::module &m) {
  declare_sparse_container<double>(m, "double");
  declare_sparse_container<float>(m, "float");
  declare_sparse_container<int64_t>(m, "int64_t");
}
