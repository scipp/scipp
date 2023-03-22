// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <Eigen/Geometry>

#include "scipp/variable/misc_operations.h"

#include "docstring.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::variable::geometry;

namespace py = pybind11;

void init_geometry(py::module &m) {
  auto geom_m = m.def_submodule("geometry");

  geom_m.def(
      "as_vectors",
      [](const Variable &x, const Variable &y, const Variable &z) {
        return position(x, y, z);
      },
      py::arg("x"), py::arg("y"), py::arg("z"),
      py::call_guard<py::gil_scoped_release>());

  geom_m.def(
      "rotation_matrix_from_quaternion_coeffs", [](py::array_t<double> value) {
        if (value.size() != 4)
          throw std::runtime_error("Incompatible list size: expected size 4.");
        return Eigen::Quaterniond(value.cast<std::vector<double>>().data())
            .toRotationMatrix();
      });
}
