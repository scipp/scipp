// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "bind_enum.h"
#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::units;

namespace py = pybind11;

void init_units_neutron(py::module &m) {
  bind_enum(m, "Dim", Dim::Invalid, 5);

  py::class_<units::Unit>(m, "Unit")
      .def(py::init())
      .def("__repr__",
           [](const units::Unit &u) -> std::string { return u.name(); })
      .def_property_readonly("name", &units::Unit::name,
                             "A read-only string describing the "
                             "type of unit.")
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self)
      .def(py::self / py::self)
      .def(py::self == py::self)
      .def(py::self != py::self);

  auto units = m.def_submodule("units");
  units.attr("dimensionless") = units::Unit(units::dimensionless);
  units.attr("m") = units::Unit(units::m);
  units.attr("counts") = units::Unit(units::counts);
  units.attr("s") = units::Unit(units::s);
  units.attr("kg") = units::Unit(units::kg);
  units.attr("K") = units::Unit(units::K);
  units.attr("angstrom") = units::Unit(units::angstrom);
  units.attr("meV") = units::Unit(units::meV);
  units.attr("us") = units::Unit(units::us);
}
