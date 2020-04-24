// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/variable.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(event_list<double>)
PYBIND11_MAKE_OPAQUE(event_list<float>)
PYBIND11_MAKE_OPAQUE(event_list<int64_t>)
PYBIND11_MAKE_OPAQUE(event_list<int32_t>)

template <class T>
void declare_event_list(py::module &m, const std::string &suffix) {
  auto s = py::bind_vector<event_list<T>>(
      m, std::string("event_list_") + suffix, py::buffer_protocol());
  // pybind11 currently does not find method from the base class, see
  // https://github.com/pybind/pybind11/pull/1832. We add the method manually
  // here.
  s.def("__len__", [](const event_list<T> &self) { return self.size(); });
}

void init_event_list(py::module &m) {
  declare_event_list<double>(m, "float64");
  declare_event_list<float>(m, "float32");
  declare_event_list<int64_t>(m, "int64");
  declare_event_list<int32_t>(m, "int32");
}
