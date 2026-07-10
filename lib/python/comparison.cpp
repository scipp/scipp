// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "nanobind.h"

#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/comparison.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace nb = nanobind;

template <class T> void bind_isclose(nb::module_ &m) {
  m.def(
      "isclose",
      [](const T &x, const T &y, const T &rtol, const T &atol,
         const bool equal_nan) {
        return isclose(x, y, rtol, atol,
                       equal_nan ? NanComparisons::Equal
                                 : NanComparisons::NotEqual);
      },
      nb::arg("x"), nb::arg("y"), nb::arg("rtol"), nb::arg("atol"),
      nb::arg("equal_nan"), nb::call_guard<nb::gil_scoped_release>());
}

template <typename T> void bind_identical(nb::module_ &m) {
  m.def(
      "identical",
      [](const T &x, const T &y, const bool equal_nan) {
        if (equal_nan) {
          return equals_nan(x, y);
        }
        return x == y;
      },
      nb::arg("x"), nb::arg("y"), nb::arg("equal_nan"),
      nb::call_guard<nb::gil_scoped_release>());
}

void init_comparison(nb::module_ &m) {
  bind_isclose<Variable>(m);
  bind_identical<Variable>(m);
  bind_identical<Dataset>(m);
  bind_identical<DataArray>(m);
}
