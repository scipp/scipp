// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "nanobind.h"
#include "unit.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/to_unit.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/to_unit.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace nb = nanobind;

namespace {
template <typename T> void bind_norm(nb::module_ &m) {
  m.def(
      "norm", [](const T &x) { return norm(x); }, nb::arg("x"),
      nb::call_guard<nb::gil_scoped_release>());
}

template <typename T> void bind_nan_to_num(nb::module_ &m) {
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
      nb::arg("x"), nb::kw_only(), nb::arg("nan") = std::optional<Variable>(),
      nb::arg("posinf") = std::optional<Variable>(),
      nb::arg("neginf") = std::optional<Variable>(),
      nb::call_guard<nb::gil_scoped_release>());

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
      nb::arg("x"), nb::kw_only(), nb::arg("nan") = std::optional<Variable>(),
      nb::arg("posinf") = std::optional<Variable>(),
      nb::arg("neginf") = std::optional<Variable>(), nb::arg("out"),
      nb::call_guard<nb::gil_scoped_release>());
}

template <class T> void bind_to_unit(nb::module_ &m) {
  m.def(
      "to_unit",
      [](const T &x, const ProtoUnit &unit, const bool copy) {
        return to_unit(x, unit_or_default(unit),
                       copy ? CopyPolicy::Always : CopyPolicy::TryAvoid);
      },
      nb::arg("x"), nb::arg("unit"), nb::arg("copy") = true,
      nb::call_guard<nb::gil_scoped_release>());
}

template <class T> void bind_as_const(nb::module_ &m) {
  m.def("as_const", [](const T &x) { return x.as_const(); }, nb::arg("x"));
}
} // namespace

void init_unary(nb::module_ &m) {
  bind_norm<Variable>(m);
  bind_nan_to_num<Variable>(m);
  bind_to_unit<Variable>(m);
  bind_to_unit<DataArray>(m);
  bind_as_const<Variable>(m);
  bind_as_const<DataArray>(m);
}
