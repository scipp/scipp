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
#include "py_object.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

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

void init_dtype(py::module &m) {
  py::class_<DType> PyDType(m, "DType", R"(
Representation of a data type of a Variable in Scipp.
See https://scipp.github.io/reference/dtype.html for details.

The data types ``VariableView``, ``DataArrayView``, and ``DatasetView`` are used for
objects containing binned data. They cannot be used directly to create arrays of bins.
)");
  PyDType.def(py::init([](const py::object &x) { return scipp_dtype(x); }))
      .def("__eq__",
           [](const DType &self, const py::object &other) {
             return self == scipp_dtype(other);
           })
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
    PyDType.def_property_readonly_static(
        core::dtypeNameRegistry().at(t).c_str(),
        [t](const py::object &) { return t; });
}

DType dtype_of(const py::object &x) {
  if (x.is_none()) {
    return dtype<void>;
  } else if (py::isinstance<py::buffer>(x)) {
    // Cannot use hasattr(x, "dtype") as that would catch Variables as well.
    return scipp_dtype(x.attr("dtype"));
  } else if (py::isinstance<py::bool_>(x)) {
    // bool needs to come before int because bools are instances of int.
    return core::dtype<bool>;
  } else if (py::isinstance<py::float_>(x)) {
    return core::dtype<double>;
  } else if (py::isinstance<py::int_>(x)) {
    return core::dtype<int64_t>;
  } else if (py::isinstance<py::str>(x)) {
    return core::dtype<std::string>;
  } else if (py::isinstance<variable::Variable>(x)) {
    return core::dtype<variable::Variable>;
  } else if (py::isinstance<dataset::DataArray>(x)) {
    return core::dtype<dataset::DataArray>;
  } else if (py::isinstance<dataset::Dataset>(x)) {
    return core::dtype<dataset::Dataset>;
  } else {
    return core::dtype<python::PyObject>;
  }
}

scipp::core::DType scipp_dtype(const py::dtype &type) {
  switch (type.normalized_num()) {
  case py::dtype::num_of<double>():
    return scipp::core::dtype<double>;
  case py::dtype::num_of<float>():
    return scipp::core::dtype<float>;
  case py::dtype::num_of<std::int64_t>():
    return scipp::core::dtype<std::int64_t>;
  case py::dtype::num_of<std::int32_t>():
    return scipp::core::dtype<std::int32_t>;
  case py::dtype::num_of<bool>():
    return scipp::core::dtype<bool>;
  case py::dtype::num_of<py::object>():
    return scipp::core::dtype<python::PyObject>;
  default:
    break;
  }
  // These cannot be handled with the above switch:
  if (type.kind() == DTypeKind::String)
    return scipp::core::dtype<std::string>;
  if (type.kind() == DTypeKind::Datetime) {
    return scipp::core::dtype<time_point>;
  }
  throw std::runtime_error(
      "Unsupported numpy dtype: " +
      py::str(static_cast<py::handle>(type)).cast<std::string>() +
      "\n"
      "Supported types are: bool, float32, float64,"
      " int32, int64, string, datetime64, and object");
}

scipp::core::DType dtype_from_scipp_class(const py::object &type) {
  // Using the __name__ because we would otherwise have to get a handle
  // to the Python classes for our C++ classes. And I don't know how
  // to do that. This approach can break if people (including us) pull
  // shenanigans with the classes in Python!
  if (type.attr("__name__").cast<std::string>() == "Variable") {
    return dtype<Variable>;
  } else if (type.attr("__name__").cast<std::string>() == "DataArray") {
    return dtype<DataArray>;
  } else if (type.attr("__name__").cast<std::string>() == "Dataset") {
    return dtype<Dataset>;
  } else {
    throw std::invalid_argument("Invalid dtype");
  }
}

namespace {
py::dtype to_np_dtype(const py::object &type) {
  try {
    return py::dtype::from_args(type);
  } catch (py::error_already_set &error) {
    // NumPy normally raises a TypeError, but for Variable, DataArray, it raises
    // ValueError because it sees the `.dtype` attribute and thinks that it is a
    // compatible np.dtype object. For some reason that triggers a different
    // error.
    if (error.matches(PyExc_ValueError)) {
      throw py::type_error(error.what());
    }
    throw;
  }
}
} // namespace

scipp::core::DType scipp_dtype(const py::object &type) {
  // Check None first, then native scipp Dtype, then numpy.dtype
  if (type.is_none())
    return dtype<void>;
  try {
    return type.cast<DType>();
  } catch (const py::cast_error &) {
    if (py::isinstance<py::type>(type) &&
        type.attr("__module__").cast<std::string>() == "scipp._scipp.core") {
      return dtype_from_scipp_class(type);
    }
    const auto np_dtype = to_np_dtype(type);
    if (np_dtype.kind() == DTypeKind::RawData) {
      throw std::invalid_argument(
          "Unsupported numpy dtype: raw data. This can happen when you pass a "
          "Python object instead of a class. Got dtype=`" +
          py::str(type).cast<std::string>() + '`');
    }
    return scipp_dtype(np_dtype);
  }
}

namespace {
bool is_default(const ProtoUnit &unit) {
  return std::holds_alternative<DefaultUnit>(unit);
}
} // namespace

std::tuple<scipp::core::DType, std::optional<scipp::sc_units::Unit>>
cast_dtype_and_unit(const pybind11::object &dtype, const ProtoUnit &unit) {
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

DType common_dtype(const py::object &values, const py::object &variances,
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

bool has_datetime_dtype(const py::object &obj) {
  if (py::hasattr(obj, "dtype")) {
    return obj.attr("dtype").attr("kind").cast<char>() == DTypeKind::Datetime;
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
parse_datetime_dtype(const pybind11::object &dtype) {
  if (py::isinstance<py::type>(dtype)) {
    // This handles dtype=np.datetime64, i.e. passing the class.
    return sc_units::one;
  } else if (py::hasattr(dtype, "dtype")) {
    return parse_datetime_dtype(dtype.attr("dtype"));
  } else if (py::hasattr(dtype, "name")) {
    return parse_datetime_dtype(dtype.attr("name").cast<std::string>());
  } else {
    return parse_datetime_dtype(py::str(dtype).cast<std::string>());
  }
}
