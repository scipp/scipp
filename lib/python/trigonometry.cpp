// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "nanobind.h"

#include "scipp/variable/trigonometry.h"

using namespace scipp;
using namespace scipp::variable;

namespace nb = nanobind;

template <class T> void bind_atan2(nb::module_ &m) {
  m.def(
      "atan2", [](const T &y, const T &x) { return atan2(y, x); },
      nb::kw_only(), nb::arg("y"), nb::arg("x"),
      nb::call_guard<nb::gil_scoped_release>());
  m.def(
      "atan2", [](const T &y, const T &x, T &out) { return atan2(y, x, out); },
      nb::kw_only(), nb::arg("y"), nb::arg("x"), nb::arg("out"),
      nb::call_guard<nb::gil_scoped_release>());
}

void init_trigonometry(nb::module_ &m) { bind_atan2<Variable>(m); }
