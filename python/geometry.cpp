// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
      "position",
      [](const Variable &x, const Variable &y, const Variable &z) {
        return position(x, y, z);
      },
      py::arg("x"), py::arg("y"), py::arg("z"),
      py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description(
              "Element-wise zip functionality to produce a vector_3_float64.")
          .raises("If the dtypes of inputs are not double precision floats.")
          .seealso(":py:func:`scipp.geometry.x`, :py:func:`scipp.geometry.y`, "
                   ":py:func:`scipp.geometry.z`")
          .returns(
              "Zip of input x, y and z. Output unit is same as input unit.")
          .rtype("Variable")
          .param("x", "Variable containing x component.", "Variable")
          .param("y", "Variable containing y component.", "Variable")
          .param("z", "Variable containing z component.", "Variable")
          .c_str());

  geom_m.def(
      "rotation_matrix_from_quaternion_coeffs", [](py::array_t<double> value) {
        if (value.size() != 4)
          throw std::runtime_error("Incompatible list size: expected size 4.");
        return Eigen::Quaterniond(value.cast<std::vector<double>>().data())
            .toRotationMatrix();
      });
}
