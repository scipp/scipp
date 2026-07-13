// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dtype.h"

#include <iostream>
#include <regex>

#include "scipp/core/eigen.h"
#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable.h"

#include "format.h"
#include "nanobind.h"
#include "py_object.h"

using namespace scipp;
using namespace scipp::core;

namespace nb = nanobind;

namespace {
/// 'kind' character codes for numpy dtypes
enum class DTypeKind : char {
  Float = 'f',
  Int = 'i',
  Bool = 'b',
  Datetime = 'M',
  Object = 'O',
  String = 'U',
  RawData = 'V',
};

constexpr bool operator==(const char a, const DTypeKind b) {
  return a == static_cast<char>(b);
}

enum class DTypeSize : scipp::index {
  Float64 = 8,
  Float32 = 4,
  Int64 = 8,
  Int32 = 4,
};

constexpr bool operator==(const scipp::index a, const DTypeSize b) {
  return a == static_cast<scipp::index>(b);
}
} // namespace

void init_dtype(nb::module_ &m) {
  nb::class_<DType> PyDType(m, "DType", R"(
Representation of a data type of a Variable in Scipp.
See https://scipp.github.io/reference/dtype.html for details.

The data types ``VariableView``, ``DataArrayView``, and ``DatasetView`` are used for
objects containing binned data. They cannot be used directly to create arrays of bins.
)");
  PyDType
      .def(nb::new_([](const nb::object &x) { return scipp_dtype(x); }),
           nb::arg("x"))
      .def("__copy__", [](const DType &self) { return self; })
      .def("__deepcopy__",
           [](const DType &self, const nb::dict &) { return self; })
      .def(
          "__eq__",
          [](const DType &self, const nb::object &other) {
            return self == scipp_dtype(other);
          },
          // scipp_dtype maps None to dtype<void>, i.e. `dtype == None` is
          // False. Without the `none` annotation nanobind would reject None
          // before the lambda runs.
          nb::arg("other").none())
      .def("__str__", [](const DType &self) { return to_string(self); })
      .def("__repr__", [](const DType &self) {
        return "DType('" + to_string(self) + "')";
      });

  // Explicit list of dtypes to bind since core::dtypeNameRegistry contains
  // types that are for internal use only and are never returned to Python.
  for (const auto &t : {
           dtype<bool>,
           dtype<int32_t>,
           dtype<int64_t>,
           dtype<float>,
           dtype<double>,
           dtype<std::string>,
           dtype<Eigen::Vector3d>,
           dtype<Eigen::Matrix3d>,
           dtype<Eigen::Affine3d>,
           dtype<core::Quaternion>,
           dtype<core::Translation>,
           dtype<core::time_point>,
           dtype<Variable>,
           dtype<DataArray>,
           dtype<Dataset>,
           dtype<core::bin<Variable>>,
           dtype<core::bin<DataArray>>,
           dtype<core::bin<Dataset>>,
           dtype<python::PyObject>,
       })
    PyDType.def_prop_ro_static(core::dtypeNameRegistry().at(t).c_str(),
                               [t](const nb::object &) { return t; });
}

DType dtype_of(const nb::object &x) {
  if (x.is_none()) {
    return dtype<void>;
  } else if (is_buffer_like(x)) {
    // Cannot use hasattr(x, "dtype") as that would catch Variables as well.
    return scipp_dtype(x.attr("dtype"));
  } else if (nb::isinstance<nb::bool_>(x)) {
    // bool needs to come before int because bools are instances of int.
    return core::dtype<bool>;
  } else if (nb::isinstance<nb::float_>(x)) {
    return core::dtype<double>;
  } else if (nb::isinstance<nb::int_>(x)) {
    return core::dtype<int64_t>;
  } else if (nb::isinstance<nb::str>(x)) {
    return core::dtype<std::string>;
  } else if (nb::isinstance<variable::Variable>(x)) {
    return core::dtype<variable::Variable>;
  } else if (nb::isinstance<dataset::DataArray>(x)) {
    return core::dtype<dataset::DataArray>;
  } else if (nb::isinstance<dataset::Dataset>(x)) {
    return core::dtype<dataset::Dataset>;
  } else {
    return core::dtype<python::PyObject>;
  }
}

namespace {
/// Map an instance of numpy.dtype to a scipp DType. Uses the 'kind' and
/// 'itemsize' attributes, which normalize platform-dependent aliases such as
/// long vs longlong.
scipp::core::DType scipp_dtype_from_np(const nb::object &type) {
  const auto kind = nb::cast<char>(type.attr("kind"));
  const auto itemsize = nb::cast<scipp::index>(type.attr("itemsize"));
  if (kind == DTypeKind::Float) {
    if (itemsize == DTypeSize::Float64)
      return scipp::core::dtype<double>;
    if (itemsize == DTypeSize::Float32)
      return scipp::core::dtype<float>;
  } else if (kind == DTypeKind::Int) {
    if (itemsize == DTypeSize::Int64)
      return scipp::core::dtype<std::int64_t>;
    if (itemsize == DTypeSize::Int32)
      return scipp::core::dtype<std::int32_t>;
  } else if (kind == DTypeKind::Bool) {
    return scipp::core::dtype<bool>;
  } else if (kind == DTypeKind::Object) {
    return scipp::core::dtype<python::PyObject>;
  } else if (kind == DTypeKind::String) {
    return scipp::core::dtype<std::string>;
  } else if (kind == DTypeKind::Datetime) {
    return scipp::core::dtype<time_point>;
  }
  throw std::runtime_error(
      "Unsupported numpy dtype: " + nb::cast<std::string>(nb::str(type)) +
      "\n"
      "Supported types are: bool, float32, float64,"
      " int32, int64, string, datetime64, and object");
}
} // namespace

scipp::core::DType dtype_from_scipp_class(const nb::object &type) {
  // Using the __name__ because we would otherwise have to get a handle
  // to the Python classes for our C++ classes. And I don't know how
  // to do that. This approach can break if people (including us) pull
  // shenanigans with the classes in Python!
  if (nb::cast<std::string>(type.attr("__name__")) == "Variable") {
    return dtype<Variable>;
  } else if (nb::cast<std::string>(type.attr("__name__")) == "DataArray") {
    return dtype<DataArray>;
  } else if (nb::cast<std::string>(type.attr("__name__")) == "Dataset") {
    return dtype<Dataset>;
  } else {
    throw std::invalid_argument("Invalid dtype");
  }
}

namespace {
nb::object to_np_dtype(const nb::object &type) {
  try {
    return nb::module_::import_("numpy").attr("dtype")(type);
  } catch (nb::python_error &error) {
    // NumPy normally raises a TypeError, but for Variable, DataArray, it raises
    // ValueError because it sees the `.dtype` attribute and thinks that it is a
    // compatible np.dtype object. For some reason that triggers a different
    // error.
    if (error.matches(PyExc_ValueError)) {
      throw nb::type_error(error.what());
    }
    throw;
  }
}
} // namespace

scipp::core::DType scipp_dtype(const nb::object &type) {
  // Check None first, then native scipp Dtype, then numpy.dtype
  if (type.is_none())
    return dtype<void>;
  if (DType out; nb::try_cast<DType>(type, out, false)) {
    return out;
  }
  if (nb::isinstance<nb::type_object>(type) &&
      nb::cast<std::string>(type.attr("__module__")) == "scipp._scipp.core") {
    return dtype_from_scipp_class(type);
  }
  const auto np_dtype = to_np_dtype(type);
  if (nb::cast<char>(np_dtype.attr("kind")) == DTypeKind::RawData) {
    throw std::invalid_argument(
        "Unsupported numpy dtype: raw data. This can happen when you pass a "
        "Python object instead of a class. Got dtype=`" +
        nb::cast<std::string>(nb::str(type)) + '`');
  }
  return scipp_dtype_from_np(np_dtype);
}

namespace {
bool is_default(const ProtoUnit &unit) {
  return std::holds_alternative<DefaultUnit>(unit);
}
} // namespace

std::tuple<scipp::core::DType, std::optional<scipp::sc_units::Unit>>
cast_dtype_and_unit(const nanobind::object &dtype, const ProtoUnit &unit) {
  const auto scipp_dtype = ::scipp_dtype(dtype);
  if (scipp_dtype == core::dtype<core::time_point>) {
    sc_units::Unit deduced_unit = parse_datetime_dtype(dtype);
    if (!is_default(unit)) {
      const auto unit_ = unit_or_default(unit, scipp_dtype);
      if (deduced_unit != sc_units::one && unit_ != deduced_unit) {
        throw std::invalid_argument(
            python::format("The unit encoded in the dtype (", deduced_unit,
                           ") conflicts with the given unit (", unit_, ")."));
      } else {
        deduced_unit = unit_;
      }
    }
    return std::tuple{scipp_dtype, deduced_unit};
  } else {
    // Concrete dtype not known at this point so we cannot determine the default
    // unit here. Therefore nullopt is returned.
    return std::tuple{scipp_dtype, is_default(unit)
                                       ? std::optional<scipp::sc_units::Unit>()
                                       : unit_or_default(unit)};
  }
}

void ensure_conversion_possible(const DType from, const DType to,
                                const std::string &data_name) {
  if (from == to || (core::is_fundamental(from) && core::is_fundamental(to)) ||
      to == dtype<python::PyObject> ||
      (core::is_int(from) && to == dtype<core::time_point>)) {
    return; // These are allowed.
  }
  throw std::invalid_argument(python::format("Cannot convert ", data_name,
                                             " from type ", from, " to ", to));
}

DType common_dtype(const nb::object &values, const nb::object &variances,
                   const DType dtype, const DType default_dtype) {
  const DType values_dtype = dtype_of(values);
  const DType variances_dtype = dtype_of(variances);
  if (dtype == core::dtype<void>) {
    // Get dtype solely from data.
    if (values_dtype == core::dtype<void>) {
      if (variances_dtype == core::dtype<void>) {
        return default_dtype;
      }
      return variances_dtype;
    } else {
      if (variances_dtype != core::dtype<void> &&
          values_dtype != variances_dtype) {
        throw std::invalid_argument(python::format(
            "The dtypes of the 'values' (", values_dtype, ") and 'variances' (",
            variances_dtype,
            ") arguments do not match. You can specify a dtype explicitly to"
            " trigger a conversion if applicable."));
      }
      return values_dtype;
    }
  } else { // dtype != core::dtype<void>
    // Combine data and explicit dtype with potential conversion.
    if (values_dtype != core::dtype<void>) {
      ensure_conversion_possible(values_dtype, dtype, "values");
    }
    if (variances_dtype != core::dtype<void>) {
      ensure_conversion_possible(variances_dtype, dtype, "variances");
    }
    return dtype;
  }
}

bool has_datetime_dtype(const nb::object &obj) {
  if (nb::hasattr(obj, "dtype")) {
    return nb::cast<char>(obj.attr("dtype").attr("kind")) ==
           DTypeKind::Datetime;
  } else {
    // numpy.datetime64 and numpy.ndarray both have 'dtype' attributes.
    // Mark everything else as not-datetime.
    return false;
  }
}

[[nodiscard]] scipp::sc_units::Unit
parse_datetime_dtype(const std::string &dtype_name) {
  static std::regex datetime_regex{R"(datetime64(\[(\w+)\])?)",
                                   std::regex_constants::optimize};
  constexpr size_t unit_idx = 2;
  std::smatch match;
  if (!std::regex_match(dtype_name, match, datetime_regex) ||
      match.size() != 3) {
    throw std::invalid_argument("Invalid dtype, expected datetime64, got " +
                                dtype_name);
  }

  if (match.length(unit_idx) == 0) {
    return scipp::sc_units::dimensionless;
  } else if (match[unit_idx] == "s") {
    return scipp::sc_units::s;
  } else if (match[unit_idx] == "us") {
    return scipp::sc_units::us;
  } else if (match[unit_idx] == "ns") {
    return scipp::sc_units::ns;
  } else if (match[unit_idx] == "m") {
    // In np.datetime64, m means minute.
    return sc_units::Unit("min");
  } else {
    for (const char *name : {"ms", "h", "D", "M", "Y"}) {
      if (match[unit_idx] == name) {
        return sc_units::Unit(name);
      }
    }
  }

  throw std::invalid_argument(std::string("Unsupported unit in datetime: ") +
                              std::string(match[unit_idx]));
}

[[nodiscard]] scipp::sc_units::Unit
parse_datetime_dtype(const nanobind::object &dtype) {
  if (nb::isinstance<nb::type_object>(dtype)) {
    // This handles dtype=np.datetime64, i.e. passing the class.
    return sc_units::one;
  } else if (nb::hasattr(dtype, "dtype")) {
    return parse_datetime_dtype(dtype.attr("dtype"));
  } else if (nb::hasattr(dtype, "name")) {
    return parse_datetime_dtype(nb::cast<std::string>(dtype.attr("name")));
  } else {
    return parse_datetime_dtype(nb::cast<std::string>(nb::str(dtype)));
  }
}
