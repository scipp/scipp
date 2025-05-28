// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/variable/transform.h"

using namespace scipp;

namespace py = pybind11;

template <class T, class... Ts> void bind_transform(py::module &m) {
  m.def("transform", [](py::object const &kernel,
                        const std::conditional_t<true, Variable, Ts> &...vars) {
    auto fptr_address = kernel.attr("address").cast<intptr_t>();
    auto fptr = reinterpret_cast<T (*)(Ts...)>(fptr_address);
    auto name = kernel.attr("name").cast<std::string>();
    return variable::transform<std::tuple<Ts...>>(
        vars...,
        overloaded{core::transform_flags::expect_no_variance_arg<0>,
                   core::transform_flags::expect_no_variance_arg<1>,
                   core::transform_flags::expect_no_variance_arg<2>,
                   core::transform_flags::expect_no_variance_arg<3>,
                   [&kernel](const sc_units::Unit &u, const auto &...us) {
                     py::gil_scoped_acquire acquire;
                     return py::cast<sc_units::Unit>(
                         kernel.attr("unit_func")(u, us...));
                   },
                   [fptr](const auto &...args) { return fptr(args...); }},
        name);
  });
}

void init_transform(py::module &m) {
  bind_transform<double, double>(m);
  bind_transform<double, double, double>(m);
  bind_transform<double, double, double, double>(m);
  bind_transform<double, double, double, double, double>(m);
}
