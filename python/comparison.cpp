// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/comparison.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

namespace detail {
template <class T, class Op>
void bind_close(std::string func_name, Op op, py::module &m) {
  m.def(
      func_name.c_str(),
      [op](const T &x, const T &y, const T &rtol, const T &atol,
           const bool equal_nan) {
        return op(x, y, rtol, atol,
                  equal_nan ? NanComparisons::Equal : NanComparisons::NotEqual);
      },
      py::arg("x"), py::arg("y"), py::arg("rtol"), py::arg("atol"),
      py::arg("equal_nan"), py::call_guard<py::gil_scoped_release>());
}
} // namespace detail

template <class T> void bind_isclose(py::module &m) {
  ::detail::bind_close<T>("isclose", isclose, m);
}
template <class T> void bind_allclose(py::module &m) {
  ::detail::bind_close<T>("allclose", allclose, m);
}

template <typename T> void bind_identical(py::module &m) {
  m.def(
      "identical", [](const T &x, const T &y) { return x == y; }, py::arg("x"),
      py::arg("y"), py::call_guard<py::gil_scoped_release>());
}

void init_comparison(py::module &m) {
  bind_isclose<Variable>(m);
  bind_allclose<Variable>(m);
  bind_identical<Variable>(m);
  bind_identical<Dataset>(m);
  bind_identical<DataArray>(m);
}
