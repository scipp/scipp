// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/to_unit.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

template <typename T> void bind_norm(py::module &m) {
  m.def(
      "norm", [](const T &x) { return norm(x); }, py::arg("x"),
      py::call_guard<py::gil_scoped_release>());
}

template <typename T> void bind_nan_to_num(py::module &m) {
  m.def(
      "nan_to_num",
      [](const T &x, const std::optional<Variable> &nan,
         const std::optional<Variable> &posinf,
         const std::optional<Variable> &neginf) {
        Variable out(x);
        if (nan)
          nan_to_num(out, *nan, out);
        if (posinf)
          positive_inf_to_num(out, *posinf, out);
        if (neginf)
          negative_inf_to_num(out, *neginf, out);
        return out;
      },
      py::arg("x"), py::arg("nan") = std::optional<Variable>(),
      py::arg("posinf") = std::optional<Variable>(),
      py::arg("neginf") = std::optional<Variable>(),
      py::call_guard<py::gil_scoped_release>());

  m.def(
      "nan_to_num",
      [](const T &x, const std::optional<Variable> &nan,
         const std::optional<Variable> &posinf,
         const std::optional<Variable> &neginf, T &out) {
        if (nan)
          nan_to_num(x, *nan, out);
        if (posinf)
          positive_inf_to_num(x, *posinf, out);
        if (neginf)
          negative_inf_to_num(x, *neginf, out);
        return out;
      },
      py::arg("x"), py::arg("nan") = std::optional<Variable>(),
      py::arg("posinf") = std::optional<Variable>(),
      py::arg("neginf") = std::optional<Variable>(), py::arg("out"),
      py::call_guard<py::gil_scoped_release>());
}

void init_unary(py::module &m) {
  bind_norm<Variable>(m);
  bind_nan_to_num<Variable>(m);
  m.def(
      "to_unit",
      [](const Variable &x, const units::Unit unit, const bool copy) {
        return variable::to_unit(
            x, unit, copy ? CopyPolicy::Always : CopyPolicy::TryAvoid);
      },
      py::arg("x"), py::arg("unit"), py::arg("copy") = true,
      py::call_guard<py::gil_scoped_release>());
}
