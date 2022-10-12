// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dtype.h"
#include "scipp/core/tag_util.h"
#include "scipp/units/unit.h"

#include "pybind11.h"
#include "unit.h"

using namespace scipp;
namespace py = pybind11;

constexpr int UNIT_DICT_VERSION = 1;

py::dict to_dict(const units::Unit &unit) {
  py::dict dict;
  dict["__version__"] = UNIT_DICT_VERSION;

  const auto &base_units = unit.underlying().base_units();
  dict["meter"] = base_units.meter();
  dict["kilogram"] = base_units.kg();
  dict["second"] = base_units.second();
  dict["ampere"] = base_units.ampere();
  dict["kelvin"] = base_units.kelvin();
  dict["mole"] = base_units.mole();
  dict["candela"] = base_units.candela();
  dict["currency"] = base_units.currency();
  dict["count"] = base_units.count();
  dict["radian"] = base_units.radian();
  dict["per_unit"] = base_units.is_per_unit();
  dict["i_flag"] = base_units.has_i_flag();
  dict["e_flag"] = base_units.has_e_flag();
  dict["equation"] = base_units.is_equation();

  // We loose some type information. commodity is uint32
  // but the dict contains a signed int.
  // This should not matter because Python's int is larger than 32 bit.
  dict["commodity"] = unit.underlying().commodity();
  dict["multiplier"] = unit.underlying().multiplier();

  return dict;
}

units::Unit from_dict(const py::dict &dict) {
  if (const auto ver = dict["__version__"].cast<int>();
      ver != UNIT_DICT_VERSION) {
    throw std::invalid_argument(
        "Unit dict has version " + std::to_string(ver) +
        " but the current installation of scipp only supports version " +
        std::to_string(UNIT_DICT_VERSION));
  }

  return units::Unit(llnl::units::precise_unit(
      llnl::units::detail::unit_data{
          dict["meter"].cast<int>(),
          dict["kilogram"].cast<int>(),
          dict["second"].cast<int>(),
          dict["ampere"].cast<int>(),
          dict["kelvin"].cast<int>(),
          dict["mole"].cast<int>(),
          dict["candela"].cast<int>(),
          dict["currency"].cast<int>(),
          dict["count"].cast<int>(),
          dict["radian"].cast<int>(),
          dict["per_unit"].cast<unsigned int>(),
          dict["i_flag"].cast<unsigned int>(),
          dict["e_flag"].cast<unsigned int>(),
          dict["equation"].cast<unsigned int>(),
      },
      dict["commodity"].cast<std::uint32_t>(),
      dict["multiplier"].cast<double>()));
}

void init_units(py::module &m) {
  py::class_<DefaultUnit>(m, "DefaultUnit")
      .def("__repr__",
           [](const DefaultUnit &) { return "<automatically deduced unit>"; });
  py::class_<units::Unit>(m, "Unit", "A physical unit.")
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
      .def("__abs__", [](const units::Unit &self) { return abs(self); })
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(
          "is_exactly_the_same",
          [](const units::Unit &self, const units::Unit &other) {
            return self.is_exactly_the_same(other);
          },
          "Check if two units are numerically identical.\n\n"
          "The regular equality operator allows for small differences "
          "in the unit's floating point multiplier. ``is_exactly_the_same`` "
          "checks for exact identity.")
      .def(hash(py::self))
      .def("to_dict", to_dict,
           "Serialize a unit to a dict.\n\nThis function is meant to be used "
           "with :meth:`scipp.Unit.from_dict` to serialize units.\n\n"
           "Warning\n"
           "-------\n"
           "The structure of the returned dict is an implementation detail and "
           "may change without warning at any time! "
           "It should not be used to access the internal representation of "
           "``Unit``.")
      .def("from_dict", from_dict,
           "Deserialize a unit from a dict.\n\nThis function is meant to be "
           "used in combination with :meth:`scipp.Unit.to_dict`.");

  m.def("abs", [](const units::Unit &u) { return abs(u); });
  m.def("pow", [](const units::Unit &u, const int64_t power) {
    return pow(u, power);
  });
  m.def("pow",
        [](const units::Unit &u, const double power) { return pow(u, power); });
  m.def("reciprocal", [](const units::Unit &u) { return units::one / u; });
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
  // Note: No binding to units::none here, use None in Python!
  units.attr("one") = units::one;
  units.attr("rad") = units::rad;
  units.attr("s") = units::s;
  units.attr("us") = units::us;
  units.attr("ns") = units::ns;
  units.attr("mm") = units::mm;

  units.attr("default_unit") = DefaultUnit{};

  m.def("to_numpy_time_string",
        py::overload_cast<const ProtoUnit &>(to_numpy_time_string));
}
