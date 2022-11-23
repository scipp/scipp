// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/sort.h"
#include "scipp/variable/math.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/slice.h"
#include "scipp/variable/sort.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

using namespace scipp;

namespace py = pybind11;

void bind_transform(py::module &m) {
  m.def("transform", [](py::object const &kernel, const Variable &a) {
    auto fptr_address = kernel.attr("address").cast<intptr_t>();
    auto fptr = reinterpret_cast<double (*)(double)>(fptr_address);
    auto name = kernel.attr("name").cast<std::string>();
    return variable::transform<double>(
        a,
        overloaded{core::transform_flags::expect_no_variance_arg<0>,
                   [&kernel](const units::Unit &x) {
                     py::gil_scoped_acquire acquire;
                     return py::cast<units::Unit>(kernel.attr("unit_func")(x));
                   },
                   [fptr](const auto &x) { return fptr(x); }},
        name);
  });

  m.def("transform", [](py::object const &kernel, const Variable &a,
                        const Variable &b) {
    auto fptr_address = kernel.attr("address").cast<intptr_t>();
    auto fptr = reinterpret_cast<double (*)(double, double)>(fptr_address);
    auto name = kernel.attr("name").cast<std::string>();
    return variable::transform<double>(
        a, b,
        overloaded{core::transform_flags::expect_no_variance_arg<0>,
                   core::transform_flags::expect_no_variance_arg<1>,
                   [&kernel](const units::Unit &x, const units::Unit &y) {
                     py::gil_scoped_acquire acquire;
                     return py::cast<units::Unit>(
                         kernel.attr("unit_func")(x, y));
                   },
                   [fptr](const auto &x, const auto &y) { return fptr(x, y); }},
        name);
  });

  m.def("transform", [](py::object const &kernel, const Variable &a,
                        const Variable &b, const Variable &c) {
    auto fptr_address = kernel.attr("address").cast<intptr_t>();
    auto fptr =
        reinterpret_cast<double (*)(double, double, double)>(fptr_address);
    auto name = kernel.attr("name").cast<std::string>();
    return variable::transform<double>(
        a, b, c,
        overloaded{core::transform_flags::expect_no_variance_arg<0>,
                   core::transform_flags::expect_no_variance_arg<1>,
                   core::transform_flags::expect_no_variance_arg<2>,
                   [&kernel](const units::Unit &x, const units::Unit &y,
                             const units::Unit &z) {
                     py::gil_scoped_acquire acquire;
                     return py::cast<units::Unit>(
                         kernel.attr("unit_func")(x, y, z));
                   },
                   [fptr](const auto &...args) { return fptr(args...); }},
        name);
  });

  m.def("transform", [](py::object const &kernel, const Variable &a,
                        const Variable &b, const Variable &c,
                        const Variable &d) {
    auto fptr_address = kernel.attr("address").cast<intptr_t>();
    auto fptr = reinterpret_cast<double (*)(double, double, double, double)>(
        fptr_address);
    auto name = kernel.attr("name").cast<std::string>();
    return variable::transform<double>(
        a, b, c, d,
        overloaded{core::transform_flags::expect_no_variance_arg<0>,
                   core::transform_flags::expect_no_variance_arg<1>,
                   core::transform_flags::expect_no_variance_arg<2>,
                   core::transform_flags::expect_no_variance_arg<3>,
                   [&kernel](const units::Unit &x, const units::Unit &y,
                             const units::Unit &z, const units::Unit &w) {
                     py::gil_scoped_acquire acquire;
                     return py::cast<units::Unit>(
                         kernel.attr("unit_func")(x, y, z, w));
                   },
                   [fptr](const auto &...args) { return fptr(args...); }},
        name);
  });
}

void init_transform(py::module &m) { bind_transform(m); }
