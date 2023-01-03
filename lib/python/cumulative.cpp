// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/variable/cumulative.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

auto cumsum_mode(const std::string &mode) {
  if (mode == "inclusive")
    return CumSumMode::Inclusive;
  if (mode == "exclusive")
    return CumSumMode::Exclusive;
  throw std::runtime_error("mode must be either 'exclusive' or 'inclusive'");
}

template <class T> void bind_cumsum(py::module &m) {
  m.def(
      "cumsum",
      [](const T &a, const std::string &mode) {
        return cumsum(a, cumsum_mode(mode));
      },
      py::arg("a"), py::arg("mode") = "inclusive",
      py::call_guard<py::gil_scoped_release>());
  m.def(
      "cumsum",
      [](const T &a, const std::string &dim, const std::string &mode) {
        return cumsum(a, Dim{dim}, cumsum_mode(mode));
      },
      py::arg("a"), py::arg("dim"), py::arg("mode") = "inclusive",
      py::call_guard<py::gil_scoped_release>());
}

void init_cumulative(py::module &m) { bind_cumsum<Variable>(m); }
