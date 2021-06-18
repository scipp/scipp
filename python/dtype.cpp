// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dtype.h"

#include <regex>

#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable.h"

#include "format.h"
#include "py_object.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

void init_dtype(py::module &m) {
  py::class_<DType>(m, "_DType")
      .def(py::self == py::self)
      .def("__repr__", [](const DType self) { return to_string(self); });
  auto dtype = m.def_submodule("dtype");
  for (const auto &[key, name] : core::dtypeNameRegistry()) {
    dtype.attr(name.c_str()) = key;
  }
}

DType dtype_of(const py::object &x) {
  if (x.is_none()) {
    return dtype<void>;
  } else if (py::isinstance<py::buffer>(x)) {
    // Cannot use hasattr(x, "dtype") as that would catch Variables as well.
    return scipp_dtype(x.attr("dtype"));
  } else if (py::isinstance<py::float_>(x)) {
    return core::dtype<double>;
  } else if (py::isinstance<py::int_>(x)) {
    return core::dtype<int64_t>;
  } else if (py::isinstance<py::bool_>(x)) {
    return core::dtype<bool>;
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

DType cast_dtype(const py::object &dtype) {
  // Check None first, then native scipp Dtype, then numpy.dtype
  if (dtype.is_none())
    return core::dtype<void>;
  try {
    return dtype.cast<DType>();
  } catch (const py::cast_error &) {
    return scipp_dtype(py::dtype::from_args(dtype));
  }
}

namespace {
const char *plural_s(const bool plural) { return plural ? "s" : ""; }
} // namespace

void ensure_conversion_possible(const DType from, const DType to,
                                const std::string &data_name) {
  if (from == to || (core::is_fundamental(from) && core::is_fundamental(to)) ||
      to == dtype<python::PyObject> ||
      (core::is_int(from) && to == dtype<core::time_point>)) {
    return; // These are allowed.
  }
  throw std::invalid_argument(
      format("Cannot convert ", data_name, " from type ", from, " to ", to));
}

DType common_dtype(const py::object &values, const py::object &variances,
                   const DType dtype, const bool plural) {
  const DType values_dtype = dtype_of(values);
  const DType variances_dtype = dtype_of(variances);
  if (dtype == core::dtype<void>) {
    // Get dtype solely from data.
    if (values_dtype == core::dtype<void>) {
      if (variances_dtype == core::dtype<void>) {
        // This would be an error by the caller of this function, not the user.
        throw std::invalid_argument("Unable to deduce a dtype");
      }
      return variances_dtype;
    } else {
      if (variances_dtype != core::dtype<void> &&
          values_dtype != variances_dtype) {
        throw std::invalid_argument(format(
            "The dtypes of the value", plural_s(plural), " (", values_dtype,
            ") and the variance", plural_s(plural), " (", variances_dtype,
            ") do not match. You can specify a dtype explicitly to trigger a "
            "conversion if applicable."));
      }
      return values_dtype;
    }
  } else { // dtype != core::dtype<void>
    // Combine data and explicit dtype with potential conversion.
    if (values_dtype != core::dtype<void>) {
      ensure_conversion_possible(values_dtype, dtype,
                                 std::string("value") + plural_s(plural));
    }
    if (variances_dtype != core::dtype<void>) {
      ensure_conversion_possible(variances_dtype, dtype,
                                 std::string("variance") + plural_s(plural));
    }
    return dtype;
  }
}

scipp::core::DType scipp_dtype(const py::dtype &type) {
  if (type.is(py::dtype::of<double>()))
    return scipp::core::dtype<double>;
  if (type.is(py::dtype::of<float>()))
    return scipp::core::dtype<float>;
  // See https://github.com/pybind/pybind11/pull/1329, int64_t not
  // matching numpy.int64 correctly.
  if (type.is(py::dtype::of<std::int64_t>()) ||
      (type.kind() == 'i' && type.itemsize() == 8))
    return scipp::core::dtype<int64_t>;
  if (type.is(py::dtype::of<std::int32_t>()) ||
      (type.kind() == 'i' && type.itemsize() == 4))
    return scipp::core::dtype<int32_t>;
  if (type.is(py::dtype::of<bool>()))
    return scipp::core::dtype<bool>;
  if (type.kind() == 'U')
    return scipp::core::dtype<std::string>;
  if (type.kind() == 'M') {
    return scipp::core::dtype<scipp::core::time_point>;
  }
  throw std::runtime_error(
      "Unsupported numpy dtype: " +
      py::str(static_cast<py::handle>(type)).cast<std::string>() +
      "\n"
      "Supported types are: bool, float32, float64,"
      " int32, int64, string, and datetime64");
}

scipp::core::DType scipp_dtype(const py::object &type) {
  // Check None first, then native scipp Dtype, then numpy.dtype
  if (type.is_none())
    return dtype<void>;
  try {
    return type.cast<DType>();
  } catch (const py::cast_error &) {
    return scipp_dtype(py::dtype::from_args(type));
  }
}

bool has_datetime_dtype(const py::object &obj) {
  if (py::hasattr(obj, "dtype")) {
    return obj.attr("dtype").attr("kind").cast<char>() == 'M';
  } else {
    // numpy.datetime64 and numpy.ndarray both have 'dtype' attributes.
    // Mark everything else as not-datetime.
    return false;
  }
}

[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const std::string &dtype_name) {
  static std::regex datetime_regex{R"(datetime64(\[(\w+)\])?)"};
  constexpr size_t unit_idx = 2;
  std::smatch match;
  if (!std::regex_match(dtype_name, match, datetime_regex) ||
      match.size() != 3) {
    throw std::invalid_argument("Invalid dtype, expected datetime64, got " +
                                dtype_name);
  }

  if (match.length(unit_idx) == 0) {
    return scipp::units::dimensionless;
  } else if (match[unit_idx] == "s") {
    return scipp::units::s;
  } else if (match[unit_idx] == "us") {
    return scipp::units::us;
  } else if (match[unit_idx] == "ns") {
    return scipp::units::ns;
  } else if (match[unit_idx] == "m") {
    // In np.datetime64, m means minute.
    return units::Unit("min");
  } else {
    for (const char *name : {"ms", "h", "D", "M", "Y"}) {
      if (match[unit_idx] == name) {
        return units::Unit(name);
      }
    }
  }

  throw std::invalid_argument(std::string("Unsupported unit in datetime: ") +
                              std::string(match[unit_idx]));
}

[[nodiscard]] scipp::units::Unit
parse_datetime_dtype(const pybind11::object &dtype) {
  if (py::isinstance<py::type>(dtype)) {
    // This handles dtype=np.datetime64, i.e. passing the class.
    return units::one;
  } else if (py::hasattr(dtype, "dtype")) {
    return parse_datetime_dtype(dtype.attr("dtype"));
  } else if (py::hasattr(dtype, "name")) {
    return parse_datetime_dtype(dtype.attr("name").cast<std::string>());
  } else {
    return parse_datetime_dtype(py::str(dtype).cast<std::string>());
  }
}
