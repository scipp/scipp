// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <Eigen/Geometry>

#include "scipp/variable/misc_operations.h"

#include "docstring.h"
#include "nanobind.h"

using namespace scipp;
using namespace scipp::variable::geometry;

namespace nb = nanobind;

void init_geometry(nb::module_ &m) {
  auto geom_m = m.def_submodule("geometry");

  geom_m.def(
      "as_vectors",
      [](const Variable &x, const Variable &y, const Variable &z) {
        return position(x, y, z);
      },
      nb::arg("x"), nb::arg("y"), nb::arg("z"),
      nb::call_guard<nb::gil_scoped_release>());

  geom_m.def("rotation_matrix_from_quaternion_coeffs",
             [](const std::vector<double> &coeffs) {
               if (coeffs.size() != 4)
                 throw std::runtime_error(
                     "Incompatible list size: expected size 4.");
               return Eigen::Quaterniond(coeffs.data()).toRotationMatrix();
             });
}
