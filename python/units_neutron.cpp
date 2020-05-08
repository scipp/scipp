// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dtype.h"
#include "scipp/core/tag_util.h"
#include "scipp/units/unit.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/variable.h"

#include "dtype.h"
#include "pybind11.h"

using namespace scipp;
namespace py = pybind11;

template <class T> struct MultScalarUnit {
  static Variable apply(const py::object &scalar, const units::Unit &unit) {
    return py::cast<T>(scalar) * unit;
  }
};

Variable doMultScalarUnit(const units::Unit &unit, const py::object &scalar,
                          const py::dtype &type) {
  return scipp::core::CallDType<double, float, int64_t, int32_t>::apply<
      MultScalarUnit>(scipp_dtype(type), scalar, unit);
}

template <class T> struct DivScalarUnit {
  static Variable apply(const py::object &scalar, const units::Unit &unit) {
    return py::cast<T>(scalar) / unit;
  }
};

Variable doDivScalarUnit(const units::Unit &unit, const py::object &scalar,
                         const py::dtype &type) {
  return scipp::core::CallDType<double, float, int64_t, int32_t>::apply<
      DivScalarUnit>(scipp_dtype(type), scalar, unit);
}

void init_units_neutron(py::module &m) {
  py::class_<units::Dim>(m, "Dim", "Dimension label")
      .def(py::init<const std::string &>())
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(hash(py::self))
      .def("__repr__", [](const Dim &dim) { return dim.name(); })
      // Pre-defined labels are temporarily useful for refactoring and may be
      // removed later.
      .def_property_readonly_static(
          "Detector", [](const py::object &) { return Dim(Dim::Detector); })
      .def_property_readonly_static(
          "DSpacing", [](const py::object &) { return Dim(Dim::DSpacing); })
      .def_property_readonly_static(
          "Energy", [](const py::object &) { return Dim(Dim::Energy); })
      .def_property_readonly_static(
          "EnergyTransfer",
          [](const py::object &) { return Dim(Dim::EnergyTransfer); })
      .def_property_readonly_static(
          "Group", [](const py::object &) { return Dim(Dim::Group); })
      .def_property_readonly_static(
          "Invalid", [](const py::object &) { return Dim(Dim::Invalid); })
      .def_property_readonly_static(
          "Position", [](const py::object &) { return Dim(Dim::Position); })
      .def_property_readonly_static(
          "Q", [](const py::object &) { return Dim(Dim::Q); })
      .def_property_readonly_static(
          "Qx", [](const py::object &) { return Dim(Dim::Qx); })
      .def_property_readonly_static(
          "Qy", [](const py::object &) { return Dim(Dim::Qy); })
      .def_property_readonly_static(
          "Qz", [](const py::object &) { return Dim(Dim::Qz); })
      .def_property_readonly_static(
          "QSquared", [](const py::object &) { return Dim(Dim::QSquared); })
      .def_property_readonly_static(
          "Row", [](const py::object &) { return Dim(Dim::Row); })
      .def_property_readonly_static(
          "ScatteringAngle",
          [](const py::object &) { return Dim(Dim::ScatteringAngle); })
      .def_property_readonly_static(
          "Spectrum", [](const py::object &) { return Dim(Dim::Spectrum); })
      .def_property_readonly_static(
          "Temperature",
          [](const py::object &) { return Dim(Dim::Temperature); })
      .def_property_readonly_static(
          "Time", [](const py::object &) { return Dim(Dim::Time); })
      .def_property_readonly_static(
          "Tof", [](const py::object &) { return Dim(Dim::Tof); })
      .def_property_readonly_static(
          "Wavelength", [](const py::object &) { return Dim(Dim::Wavelength); })
      .def_property_readonly_static(
          "X", [](const py::object &) { return Dim(Dim::X); })
      .def_property_readonly_static(
          "Y", [](const py::object &) { return Dim(Dim::Y); })
      .def_property_readonly_static(
          "Z", [](const py::object &) { return Dim(Dim::Z); });

  py::class_<units::Unit>(m, "Unit", "A physical unit.")
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
      .def("__pow__",
           [](const units::Unit &self, int power) -> units::Unit {
             switch (power) {
             case 0:
               return units::one;
             case 1:
               return self;
             case 2:
               return self * self;
             case 3:
               return self * self * self;
             case -1:
               return units::Unit() / (self);
             case -2:
               return units::Unit() / (self * self);
             case -3:
               return units::Unit() / (self * self * self);
             default:
               throw std::runtime_error("Unsupported power of unit.");
             }
           })
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("__rmul",
           [](const units::Unit &self, double scalar) { return scalar * self; })
      .def("__rmul", [](const units::Unit &self,
                        int64_t scalar) { return scalar * self; })
      .def("__rmul", &doMultScalarUnit)
      .def("__rtruediv",
           [](const units::Unit &self, double scalar) { return scalar / self; })
      .def("__rtruediv", [](const units::Unit &self,
                            int64_t scalar) { return scalar / self; })
      .def("__rtruediv", &doDivScalarUnit);

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
}
