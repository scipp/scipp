// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dtype.h"
#include "scipp/core/tag_util.h"
#include "scipp/units/unit.h"
#include "scipp/variable/variable.h"

#include "dtype.h"
#include "numpy.h"
#include "pybind11.h"
#include "unit.h"

using namespace scipp;
namespace py = pybind11;

void init_units(py::module &m) {
  py::class_<units::Dim>(m, "Dim", "Dimension label")
      .def(py::init<const std::string &>())
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(hash(py::self))
      .def("__repr__", [](const Dim &dim) { return dim.name(); });

  py::class_<units::Unit>(m, "Unit", "A physical unit.")
      .def(py::init())
      .def(py::init<const std::string &>())
      .def("__repr__", [](const units::Unit &u) { return u.name(); })
      .def_property_readonly("name", &units::Unit::name,
                             "A read-only string describing the "
                             "type of unit.")
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self)
      // cppcheck-suppress duplicateExpression
      .def(py::self / py::self)
      .def("__pow__", [](const units::Unit &self,
                         const int64_t power) { return pow(self, power); })
      .def(py::self == py::self)
      .def(py::self != py::self);

  m.def("sqrt", [](const units::Unit &u) { return sqrt(u); });

  py::implicitly_convertible<std::string, units::Unit>();

  auto units = m.def_submodule("units");
  units.attr("angstrom") = units::angstrom;
  units.attr("counts") = units::counts;
  units.attr("deg") = units::deg;
  units.attr("dimensionless") = units::dimensionless;
  units.attr("kg") = units::kg;
  units.attr("K") = units::K;
  units.attr("meV") = units::meV;
  units.attr("m") = units::m;
  units.attr("one") = units::one;
  units.attr("rad") = units::rad;
  units.attr("s") = units::s;
  units.attr("us") = units::us;
  units.attr("ns") = units::ns;
  units.attr("mm") = units::mm;
}
