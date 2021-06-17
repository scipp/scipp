#include "pybind11.h"

#include "scipp/core/dtype.h"
#include "scipp/core/eigen.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable.h"

#include "dtype.h"
#include "numpy.h"
#include "py_object.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

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
  } else if (py::isinstance<Variable>(x)) {
    return core::dtype<Variable>;
  } else if (py::isinstance<DataArray>(x)) {
    return core::dtype<DataArray>;
  } else if (py::isinstance<Dataset>(x)) {
    return core::dtype<Dataset>;
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

// TODO replace scipp_dtype
std::tuple<DType, units::Unit>
cast_dtype_and_unit(const py::object &dtype,
                    const std::optional<units::Unit> unit) {
  const auto scipp_dtype = cast_dtype(dtype);
  if (scipp_dtype == core::dtype<core::time_point>) {
    const units::Unit deduced_unit = parse_datetime_dtype(dtype);
    if (unit.has_value() && *unit != deduced_unit) {
      throw std::invalid_argument(
          "The unit encoded in the dtype (" + to_string(deduced_unit) +
          ") conflicts with the explicitly specified unit (" +
          to_string(*unit) + ").");
    }
    return std::tuple{scipp_dtype, deduced_unit};
  } else {
    return std::tuple{scipp_dtype, unit.value_or(units::one)};
  }
}

Dimensions build_dimensions(const py::object &labels, const py::object &shape) {
  if (labels.is_none()) {
    if (!shape.is_none()) {
      throw std::invalid_argument(
          "Cannot determine dimensions, got a shape but no dimension labels.");
    }
    return Dimensions{};
  } else {
    if (shape.is_none()) {
      throw std::invalid_argument(
          "Cannot determine dimensions, got dimension labels but no shape.");
    }
    return Dimensions(labels.cast<std::vector<Dim>>(),
                      shape.cast<std::vector<scipp::index>>());
  }
}

template <class T> struct MakeVariableDefaultInit {
  static Variable apply(const Dimensions &dims, const units::Unit unit,
                        const std::optional<bool> with_variance) {
    auto var = with_variance.template value_or(false)
                   ? makeVariable<T>(Dimensions{dims}, Values{}, Variances{})
                   : makeVariable<T>(Dimensions{dims});
    var.setUnit(unit);
    return var;
  }
};

Variable make_variable_default_init(const py::object &dim_labels,
                                    const py::object &shape,
                                    const units::Unit unit, DType dtype,
                                    const std::optional<bool> with_variance) {
  const auto dims = build_dimensions(dim_labels, shape);
  if (dtype == core::dtype<void>) {
    // When there is no way to deduce the dtype, default to double.
    dtype = core::dtype<double>;
  }
  return core::CallDType<
      double, float, int64_t, int32_t, bool, scipp::core::time_point,
      std::string, Variable, DataArray, Dataset, Eigen::Vector3d,
      Eigen::Matrix3d,
      python::PyObject>::apply<MakeVariableDefaultInit>(dtype, dims, unit,
                                                        with_variance);
}

const char *plural_s(const bool plural) { return plural ? "s" : ""; }

void ensure_conversion_possible(const DType from, const DType to,
                                const std::string &data_name) {
  if (from != to &&
      (!core::is_fundamental(from) || !core::is_fundamental(to))) {
    throw std::invalid_argument("Cannot convert " + data_name + " from type " +
                                to_string(from) + " to " + to_string(to));
  }
}

DType common_dtype(const py::object &values, const py::object &variances,
                   const DType dtype, const bool plural) {
  const DType values_dtype = dtype_of(values);
  const DType variances_dtype = dtype_of(variances);
  if (dtype == core::dtype<void>) {
    // Get dtype solely from data.
    if (values_dtype == core::dtype<void>) {
      if (variances_dtype == core::dtype<void>) {
        // This would be an error of the caller of this function, not the user.
        throw std::invalid_argument("Unable to deduce a dtype");
      }
      return variances_dtype;
    } else {
      if (variances_dtype != core::dtype<void> &&
          values_dtype != variances_dtype) {
        throw std::invalid_argument(
            std::string("The dtypes of the value") + plural_s(plural) + " (" +
            to_string(values_dtype) + ") and the variance" + plural_s(plural) +
            " (" + to_string(variances_dtype) +
            ") do not match. You can specify a dtype explicitly to trigger a "
            "conversion if applicable.");
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

void assert_is_scalar(const py::buffer &array) {
  if (const auto ndim = array.attr("ndim").cast<int64_t>(); ndim != 0) {
    throw std::invalid_argument("Cannot interpret " + std::to_string(ndim) +
                                "-dimensional array as a scalar.");
  }
}

template <class T>
T extract_scalar(const py::object &obj, const units::Unit unit) {
  using TM = ElementTypeMap<T>;
  using PyType = typename TM::PyType;
  TM::check_assignable(obj, unit);
  if (py::isinstance<py::buffer>(obj)) {
    assert_is_scalar(obj);
    return obj.attr("item")().cast<PyType>();
  } else {
    return obj.cast<PyType>();
  }
}

template <>
core::time_point extract_scalar<core::time_point>(const py::object &obj,
                                                  const units::Unit unit) {
  using TM = ElementTypeMap<core::time_point>;
  using PyType = typename TM::PyType;
  TM::check_assignable(obj, unit);
  if (py::isinstance<py::buffer>(obj)) {
    assert_is_scalar(obj);
    return core::time_point{obj.attr("astype")(py::dtype::of<PyType>())
                                .attr("item")()
                                .template cast<PyType>()};
  } else {
    return core::time_point{obj.cast<PyType>()};
  }
}

template <>
python::PyObject extract_scalar<python::PyObject>(const py::object &obj,
                                                  const units::Unit unit) {
  using TM = ElementTypeMap<core::time_point>;
  using PyType = typename TM::PyType;
  TM::check_assignable(obj, unit);
  return obj;
}

template <class T> struct MakeVariableScalar {
  static Variable apply(const py::object &value, const py::object &variance,
                        const units::Unit unit,
                        const std::optional<bool> with_variance) {
    auto variable = [&]() {
      auto val = [&]() {
        if (value.is_none()) {
          return element_array<T>();
        } else {
          return element_array<T>(1, extract_scalar<T>(value, unit));
        }
      }();
      auto var = [&]() {
        if (variance.is_none()) {
          return element_array<T>();
        } else {
          return element_array<T>(1, extract_scalar<T>(variance, unit));
        }
      }();
      if (with_variance.value_or(false)) {
        return makeVariable<T>(Values(std::move(val)),
                               Variances(std::move(var)));
      } else {
        return makeVariable<T>(Values(std::move(val)));
      }
    }();

    variable.setUnit(unit);
    return variable;
  }
};

Variable make_variable_scalar(const py::object &value,
                              const py::object &variance,
                              const units::Unit unit, DType dtype,
                              const std::optional<bool> with_variance) {
  dtype = common_dtype(value, variance, dtype, false);
  return core::CallDType<
      double, float, int64_t, int32_t, bool, scipp::core::time_point,
      std::string, Variable, DataArray, Dataset,
      python::PyObject>::apply<MakeVariableScalar>(dtype, value, variance, unit,
                                                   with_variance);
}

void bind_new_init(py::module &m, py::class_<Variable> &cls) {
  m.def(
      "make_variable",
      [](const py::object &dims, const py::object &shape,
         const py::object &values, const py::object &variances,
         const py::object &value, const py::object &variance,
         const std::optional<bool> with_variance,
         const std::optional<units::Unit> unit, const py::object &dtype) {
        const auto [scipp_dtype, actual_unit] =
            cast_dtype_and_unit(dtype, unit);

        if (!values.is_none() || !variances.is_none()) {
          if (!value.is_none() || !variance.is_none()) {
            // TODO
            throw std::invalid_argument("make up your mind");
          }
          if (dims.is_none()) {
            // TODO
            throw std::invalid_argument("need dims");
          }
          // make array (check shape)
        } else if (!value.is_none() || !variance.is_none()) {
          if (!dims.is_none()) {
            // TODO
            throw std::invalid_argument("no dims!");
          }
          if (!shape.is_none()) {
            // TODO
            throw std::invalid_argument("no shape!");
          }
          return make_variable_scalar(value, variance, actual_unit, scipp_dtype,
                                      with_variance);
        } else {
          return make_variable_default_init(dims, shape, actual_unit,
                                            scipp_dtype, with_variance);
        }
        // TODO unreachable
        return Variable{};
      },
      py::kw_only(), py::arg("dims") = py::none(),
      py::arg("shape") = py::none(), py::arg("values") = py::none(),
      py::arg("variances") = py::none(), py::arg("value") = py::none(),
      py::arg("variance") = py::none(), py::arg("with_variance") = std::nullopt,
      py::arg("unit") = std::nullopt, py::arg("dtype") = py::none(),
      R"raw(Make a Variable)raw");
}