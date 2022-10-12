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

namespace {

template <class Target = void, class T>
void store(py::dict &dict, const char *const name, T val) {
  if (val != 0) {
    dict[name] = static_cast<
        std::conditional_t<std::is_same_v<Target, void>, T, Target>>(val);
  }
}

py::dict to_dict(const units::Unit &unit) {
  py::dict dict;
  dict["__version__"] = UNIT_DICT_VERSION;

  const auto &base_units = unit.underlying().base_units();
  store(dict, "meter", base_units.meter());
  store(dict, "kilogram", base_units.kg());
  store(dict, "second", base_units.second());
  store(dict, "ampere", base_units.ampere());
  store(dict, "kelvin", base_units.kelvin());
  store(dict, "mole", base_units.mole());
  store(dict, "candela", base_units.candela());
  store(dict, "currency", base_units.currency());
  store(dict, "count", base_units.count());
  store(dict, "radian", base_units.radian());
  // Returning ints instead of bools because
  // - The constructor of unit_data takes unsigned int as arguments.
  // - h5py saves bools as enum types which waste disk space.
  store<unsigned int>(dict, "per_unit", base_units.is_per_unit());
  store<unsigned int>(dict, "i_flag", base_units.has_i_flag());
  store<unsigned int>(dict, "e_flag", base_units.has_e_flag());
  store<unsigned int>(dict, "equation", base_units.is_equation());

  // We loose some type information. commodity is uint32
  // but the dict contains a signed int.
  // This should not matter because Python's int is larger than 32 bit.
  store(dict, "commodity", unit.underlying().commodity());

  dict["multiplier"] = unit.underlying().multiplier();

  return dict;
}

template <class T> T get(const py::dict &dict, const char *const name) {
  if (dict.contains(name)) {
    return dict[name].cast<T>();
  }
  return T{};
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
          get<int>(dict, "meter"),
          get<int>(dict, "kilogram"),
          get<int>(dict, "second"),
          get<int>(dict, "ampere"),
          get<int>(dict, "kelvin"),
          get<int>(dict, "mole"),
          get<int>(dict, "candela"),
          get<int>(dict, "currency"),
          get<int>(dict, "count"),
          get<int>(dict, "radian"),
          get<unsigned int>(dict, "per_unit"),
          get<unsigned int>(dict, "i_flag"),
          get<unsigned int>(dict, "e_flag"),
          get<unsigned int>(dict, "equation"),
      },
      get<std::uint32_t>(dict, "commodity"),
      dict["multiplier"].cast<double>()));
}

} // namespace

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
