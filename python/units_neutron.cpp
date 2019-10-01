// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h" // clang-7 error "incomplete type 'Dataset' used in type trait expression" without this line
#include "scipp/core/dtype.h"
#include "scipp/core/tag_util.h"
#include "scipp/core/variable.h"
#include "scipp/units/unit.h"

#include "bind_enum.h"
#include "pybind11.h"

using namespace scipp;
namespace py = pybind11;

template <class T> struct MultScalarUnit {

  static scipp::core::Variable apply(const py::object &scalar,
                                     const units::Unit &unit) {
    using namespace scipp::core;
    return py::cast<T>(scalar) * unit;
  }
};

scipp::core::Variable doMultScalarUnit(const py::object &scalar,
                                       const py::dtype &type,
                                       const units::Unit &unit) {
  return scipp::core::CallDType<double, float, int64_t, int32_t>::apply<
      MultScalarUnit>(scippy::scipp_dtype(type), scalar, unit);
}

template <class T> struct DivScalarUnit {

  static scipp::core::Variable apply(const py::object &scalar,
                                     const units::Unit &unit) {
    using namespace scipp::core;
    return py::cast<T>(scalar) / unit;
  }
};

scipp::core::Variable doDivScalarUnit(const py::object &scalar,
                                      const py::dtype &type,
                                      const units::Unit &unit) {
  return scipp::core::CallDType<double, float, int64_t, int32_t>::apply<
      DivScalarUnit>(scippy::scipp_dtype(type), scalar, unit);
}

void init_units_neutron(py::module &m) {
  bind_enum(m, "Dim", Dim::Invalid, 4);

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
               return units::dimensionless;
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
           [](const units::Unit &self, double scalar) -> core::Variable {
             using namespace scipp::core;
             return scalar * self;
           })
      .def("__rmul",
           [](const units::Unit &self, int64_t scalar) -> core::Variable {
             using namespace scipp::core;
             return scalar * self;
           })
      .def("__rmul",
           [](const units::Unit &self, const py::object &scalar,
              const py::dtype &dtype) -> core::Variable {
             return doMultScalarUnit(scalar, dtype, self);
           })
      .def("__rtruediv",
           [](const units::Unit &self, double scalar) -> core::Variable {
             using namespace scipp::core;
             return scalar / self;
           })
      .def("__rtruediv",
           [](const units::Unit &self, int64_t scalar) -> core::Variable {
             using namespace scipp::core;
             return scalar / self;
           })
      .def("__rtruediv",
           [](const units::Unit &self, const py::object &scalar,
              const py::dtype &dtype) -> core::Variable {
             return doDivScalarUnit(scalar, dtype, self);
           });

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
  units.attr("rad") = units::Unit(units::rad);
  units.attr("deg") = units::Unit(units::deg);
}
