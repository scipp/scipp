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

template <class Function>
void bind_component(const std::string xyz, Function func, py::module &gm) {
  gm.def(
      xyz.c_str(), [func](const VariableConstView &pos) { return func(pos); },
      py::arg("pos"), py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("Un-zip functionality to produce a Variable of the " +
                       xyz + " component of a vector_3_float64.")
          .raises("If the dtype of the input is not vector_3_float64.")
          .seealso(":py:func:`scipp.geometry.x`, :py:func:`scipp.geometry.y`, "
                   ":py:func:`scipp.geometry.z`")
          .returns(
              "Extracted " + xyz +
              " component of input pos. Output unit is same as input unit.")
          .rtype("Variable")
          .param("pos", "Variable containing position vector.", "Variable")
          .c_str());
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

  bind_component("x", x, geom_m);
  bind_component("y", y, geom_m);
  bind_component("z", z, geom_m);
}
