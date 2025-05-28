// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <sstream>

#include "scipp/core/tag_util.h"
#include "scipp/units/unit.h"

#include "pybind11.h"
#include "unit.h"

using namespace scipp;
namespace py = pybind11;

constexpr int UNIT_DICT_VERSION = 2;
constexpr std::array SUPPORTED_UNIT_DICT_VERSIONS = {1, 2};

using UnitDictValue =
    py::typing::Union<int, double, bool, py::typing::Dict<py::str, double>>;
using UnitDict = py::typing::Dict<py::str, UnitDictValue>;

namespace {

bool is_supported_unit(const sc_units::Unit &unit) {
  return unit.underlying().commodity() == 0;
}

// We only support units where we are confident that we can encode them using
// a different unit library, in order to ensure that we can switch
// implementations in the future if necessary.
void assert_supported_unit_for_dict(const sc_units::Unit &unit) {
  if (!is_supported_unit(unit)) {
    throw std::invalid_argument("Unit cannot be converted to dict: '" +
                                to_string(unit) +
                                "' Commodities are not supported.");
  }
}

auto to_dict(const sc_units::Unit &unit) {
  assert_supported_unit_for_dict(unit);

  UnitDict dict;
  dict["__version__"] = UNIT_DICT_VERSION;
  dict["multiplier"] = unit.underlying().multiplier();

  unit.map_over_flags([&dict](const char *const name, const auto flag) mutable {
    if (flag) {
      dict[name] = true;
    }
  });

  py::dict powers;
  unit.map_over_bases(
      [&powers](const char *const base, const auto power) mutable {
        if (power != 0) {
          powers[base] = power;
        }
      });
  if (!powers.empty())
    dict["powers"] = powers;

  return dict;
}

template <class T = int> T get(const py::dict &dict, const char *const name) {
  if (dict.contains(name)) {
    return dict[name].cast<T>();
  }
  return T{};
}

void assert_dict_version_supported(const py::dict &dict) {
  if (const auto ver = dict["__version__"].cast<int>();
      std::find(SUPPORTED_UNIT_DICT_VERSIONS.cbegin(),
                SUPPORTED_UNIT_DICT_VERSIONS.cend(),
                ver) == SUPPORTED_UNIT_DICT_VERSIONS.cend()) {
    std::ostringstream oss;
    oss << "Unit dict has version " << std::to_string(ver)
        << " but the current installation of scipp only supports versions [";
    for (const auto v : SUPPORTED_UNIT_DICT_VERSIONS)
      oss << v << ", ";
    oss << "]";
    throw std::invalid_argument(oss.str());
  }
}

sc_units::Unit from_dict(const UnitDict &dict) {
  assert_dict_version_supported(dict);

  const py::dict powers = dict.contains("powers") ? dict["powers"] : py::dict();
  const double multiplier = dict["multiplier"].cast<double>();
  const auto unit_data = units::detail::unit_data{get(powers, "m"),
                                                  get(powers, "kg"),
                                                  get(powers, "s"),
                                                  get(powers, "A"),
                                                  get(powers, "K"),
                                                  get(powers, "mol"),
                                                  get(powers, "cd"),
                                                  get(powers, "$"),
                                                  get(powers, "counts"),
                                                  get(powers, "rad"),
                                                  get<bool>(dict, "per_unit"),
                                                  get<bool>(dict, "i_flag"),
                                                  get<bool>(dict, "e_flag"),
                                                  get<bool>(dict, "equation")};
  const auto precise_u = units::precise_unit(multiplier, unit_data);
  return sc_units::Unit(precise_u);
}

std::string repr(const sc_units::Unit &unit) {
  if (!is_supported_unit(unit)) {
    return "<unsupported unit: " + to_string(unit) + '>';
  }

  std::ostringstream oss;
  oss << "Unit(";

  bool first = true;
  if (const auto mult = unit.underlying().multiplier(); mult != 1.0) {
    oss << mult;
    first = false;
  }

  unit.map_over_bases(
      [&oss, &first](const char *const base, const auto power) mutable {
        if (power != 0) {
          if (!first) {
            oss << "*";
          } else {
            first = false;
          }
          oss << base;
          if (power != 1)
            oss << "**" << power;
        }
      });
  if (first)
    oss << "1"; // multiplier == 1 and all powers == 0

  unit.map_over_flags([&oss](const char *const name, const auto flag) mutable {
    if (flag)
      oss << ", " << name << "=True";
  });
  oss << ')';
  return oss.str();
}

std::string repr_html(const sc_units::Unit &unit) {
  // Regular string output is in a div with data-mime-type="text/plain"
  // But html output is in a div with data-mime-type="text/html"
  // Jupyter applies different padding to those, so hack the inner pre element
  // to match the padding of text/plain.
  return "<pre style=\"margin-bottom:0; padding-top:var(--jp-code-padding)\">" +
         unit.name() + "</pre>";
}

void repr_pretty(const sc_units::Unit &unit, py::object &p,
                 [[maybe_unused]] const bool cycle) {
  p.attr("text")(unit.name());
}

} // namespace

void init_units(py::module &m) {
  py::class_<DefaultUnit>(m, "DefaultUnit")
      .def("__repr__",
           [](const DefaultUnit &) { return "<automatically deduced unit>"; });
  py::class_<sc_units::Unit>(m, "Unit", "A physical unit.")
      .def(py::init<const std::string &>())
      .def("__str__", [](const sc_units::Unit &u) { return u.name(); })
      .def("__repr__", repr)
      .def("_repr_html_", repr_html)
      .def("_repr_pretty_", repr_pretty)
      .def_property_readonly("name", &sc_units::Unit::name,
                             "A read-only string describing the "
                             "type of unit.")
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self)
      // cppcheck-suppress duplicateExpression
      .def(py::self / py::self)
      .def("__pow__", [](const sc_units::Unit &self,
                         const int64_t power) { return pow(self, power); })
      .def("__abs__", [](const sc_units::Unit &self) { return abs(self); })
      .def(py::self == py::self)
      .def(py::self != py::self)
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

  m.def("abs", [](const sc_units::Unit &u) { return abs(u); });
  m.def("pow", [](const sc_units::Unit &u, const int64_t power) {
    return pow(u, power);
  });
  m.def("pow", [](const sc_units::Unit &u, const double power) {
    return pow(u, power);
  });
  m.def("reciprocal",
        [](const sc_units::Unit &u) { return sc_units::one / u; });
  m.def("sqrt", [](const sc_units::Unit &u) { return sqrt(u); });

  py::implicitly_convertible<std::string, sc_units::Unit>();

  auto units = m.def_submodule("units");
  units.attr("angstrom") = sc_units::angstrom;
  units.attr("counts") = sc_units::counts;
  units.attr("deg") = sc_units::deg;
  units.attr("dimensionless") = sc_units::dimensionless;
  units.attr("kg") = sc_units::kg;
  units.attr("K") = sc_units::K;
  units.attr("meV") = sc_units::meV;
  units.attr("m") = sc_units::m;
  // Note: No binding to units::none here, use None in Python!
  units.attr("one") = sc_units::one;
  units.attr("rad") = sc_units::rad;
  units.attr("s") = sc_units::s;
  units.attr("us") = sc_units::us;
  units.attr("ns") = sc_units::ns;
  units.attr("mm") = sc_units::mm;

  units.attr("default_unit") = DefaultUnit{};

  m.def("to_numpy_time_string",
        py::overload_cast<const ProtoUnit &>(to_numpy_time_string))
      .def(
          "units_identical",
          [](const sc_units::Unit &a, const sc_units::Unit &b) {
            return identical(a, b);
          },
          "Check if two units are numerically identical.\n\n"
          "The regular equality operator allows for small differences "
          "in the unit's floating point multiplier. ``units_identical`` "
          "checks for exact identity.")
      .def("add_unit_alias", scipp::sc_units::add_unit_alias, py::kw_only(),
           py::arg("name"), py::arg("unit"))
      .def("clear_unit_aliases", scipp::sc_units::clear_unit_aliases);
}
