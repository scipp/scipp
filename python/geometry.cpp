// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "bind_free_function.h"
#include "pybind11.h"

// #include "scipp/dataset/dataset.h"
// #include "scipp/dataset/sort.h"
#include "scipp/variable/misc_operations.h"

using namespace scipp;
// using namespace scipp::variable;
using namespace scipp::variable::geometry;
using namespace scipp::python;

namespace py = pybind11;

template <class T, class T1>
void bind_component(T (*func)(T1), std::string comp, py::module &m) {
  Docstring docs = {
      // Description
      "Un-zip functionality to produce a Variable of the " + comp +
          " component of a vector_3_float64.",
      // Raises
      "If the dtype of input is not vector_3_float64.",
      // See also
      ":py:class:`scipp.geometry.x`, `scipp.geometry.y`, "
      ":py:class:`scipp.geometry.z`.",
      // Returns
      "Extracted " + comp + " component of input position vector.",
      // Return type
      // "Variable.",
      // Input parameters
      {{"pos", "Variable containing position vector."}}};
  bind_free_function<T, T1>(func, comp, m, docs);
}

void init_geometry(py::module &m) {

  auto geom_m = m.def_submodule("geometry");

  using ConstView = const typename Variable::const_view_type &;
  using View = typename Variable::view_type;
  Docstring docs;

  // Position
  docs = {// Description
          "Element-wise zip functionality to produce a vector_3_float64 from "
          "independent input variables.",
          // Raises
          "If the dtypes of inputs are not double precision floats.",
          // See also
          "",
          // Returns
          "Zip of input x, y and z. Output unit is same as input unit.",
          // Return type
          // "Variable.",
          // Input parameters
          {{"x", "Variable containing x component."},
           {"y", "Variable containing y component."},
           {"z", "Variable containing z component."}}};
  bind_free_function<Variable, ConstView, ConstView, ConstView>(
      position, "position", geom_m, docs);

  // XYZ components
  bind_component<Variable, ConstView>(x, "x", geom_m);
  bind_component<Variable, ConstView>(y, "y", geom_m);
  bind_component<Variable, ConstView>(z, "z", geom_m);

  // Rotate
  docs = {// Description
          "Rotate a Variable of type vector_3_float64 using a Variable of type "
          "quaternion_float64.",
          // Raises
          "If the units of the inputs are not meter and dimensionless, for the "
          "positions and the rotations, respectively.",
          // See also
          "",
          // Returns
          " Variable containing the rotated position vectors.",
          // Return type
          // "Variable.",
          // Input parameters
          {{"position", "Variable containing xyz position vectors."},
           {"rotation", "Variable containing rotation quaternions."}}};
  bind_free_function<Variable, ConstView, ConstView>(rotate, "rotate", geom_m,
                                                     docs);
  bind_free_function<View, ConstView, ConstView, const View &>(
      rotate, "rotate", geom_m, docs.with_out_arg());
}
