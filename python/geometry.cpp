// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "docstring.h"
#include "pybind11.h"

#include "scipp/variable/misc_operations.h"

using namespace scipp;
using namespace scipp::variable::geometry;

namespace py = pybind11;

Docstring make_comp_docstring(const std::string xyz) {
  return Docstring()
    .description("Un-zip functionality to produce a Variable of the " + xyz +
                 " component of a vector_3_float64.")
    .raises("If the dtype of the input is not vector_3_float64.")
    .seealso(":py:class:`scipp.geometry.x`, :py:class:`scipp.geometry.y`, "
             ":py:class:`scipp.geometry.z`")
    .returns("Extracted " + xyz +
             " component of input pos. Output unit is same as input unit.")
    .rtype("Variable")
    .param("pos", "Variable containing position vector.");
}

void init_geometry(py::module &m) {

  auto geom_m = m.def_submodule("geometry");

  geom_m.def(
      "position",
      [](const VariableConstView &x, const VariableConstView &y,
         const VariableConstView &z) { return position(x, y, z); },
      py::arg("x"), py::arg("y"), py::arg("z"),
      py::call_guard<py::gil_scoped_release>(),
      Docstring()
      .description("Element-wise zip functionality to produce a vector_3_float64.")
      .raises("If the dtypes of inputs are not double precision floats.")
      .seealso(":py:class:`scipp.geometry.x`")
      .returns("Zip of input x, y and z. Output unit is same as input unit.")
      .rtype("Variable")
      .param("x", "Variable containing x component.")
      .param("y", "Variable containing y component.")
      .param("z", "Variable containing z component.").c_str());

  geom_m.def(
      "x", [](const VariableConstView &pos) { return x(pos); },
      py::arg("pos"),
      py::call_guard<py::gil_scoped_release>(),
      make_comp_docstring("x").c_str());

  geom_m.def(
      "y", [](const VariableConstView &pos) { return y(pos); },
      py::arg("pos"),
      py::call_guard<py::gil_scoped_release>(),
      make_comp_docstring("y").c_str());

  geom_m.def(
      "z", [](const VariableConstView &pos) { return z(pos); },
      py::arg("pos"),
      py::call_guard<py::gil_scoped_release>(),
      make_comp_docstring("z").c_str());


  auto doc = Docstring()
    .description("Rotate a Variable of type vector_3_float64 using a Variable "
                 "of type quaternion_float64.")
    .raises("If the units of the rotations are dimensionless.")
    .returns("Variable containing the rotated position vectors")
    .rtype("Variable")
    .param("pos", "Variable containing xyz position vectors.")
    .param("rot", "Variable containing rotation quaternions.");

  geom_m.def(
      "rotate",
      [](const VariableConstView &pos, const VariableConstView &rot) {
        return rotate(pos, rot);
      },
      py::arg("pos"), py::arg("rot"),
      py::call_guard<py::gil_scoped_release>(),
      doc.c_str());

  geom_m.def(
      "rotate",
      [](const VariableConstView &pos, const VariableConstView &rot,
         const VariableView &out) { return rotate(pos, rot, out); },
      py::arg("pos"), py::arg("rot"), py::arg("out"),
      doc.rtype("VariableView").param("out", "Output buffer").c_str());
}
