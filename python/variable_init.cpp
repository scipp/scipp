// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include <sstream>

#include "pybind11.h"

#include "scipp/core/dtype.h"
#include "scipp/core/eigen.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/string.h"
#include "scipp/variable/variable.h"

#include "dtype.h"
#include "format.h"
#include "numpy.h"
#include "py_object.h"
#include "unit.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

namespace {
std::tuple<DType, units::Unit>
cast_dtype_and_unit(const py::object &dtype,
                    const std::optional<units::Unit> unit) {
  const auto scipp_dtype = ::scipp_dtype(dtype);
  if (scipp_dtype == core::dtype<core::time_point>) {
    units::Unit deduced_unit = parse_datetime_dtype(dtype);
    if (unit.has_value()) {
      if (deduced_unit != units::one && *unit != deduced_unit) {
        throw std::invalid_argument(format(
            "The unit encoded in the dtype (", deduced_unit,
            ") conflicts with the explicitly specified unit (", *unit, ")."));
      } else {
        deduced_unit = *unit;
      }
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

void ensure_consistent_ndim(const std::vector<Dim> &dim_labels,
                            const py::object &shape,
                            const std::optional<py::array> &values,
                            const std::optional<py::array> &variances) {
  const auto ndim = scipp::size(dim_labels);
  const auto check_ndim = [&ndim](const std::optional<py::array> &array,
                                  const char *name) {
    if (array.has_value() && array->ndim() != ndim) {
      throw except::DimensionError(format(
          "The number of dimensions (", array->ndim(), ") of the ", name,
          " does not match the number of dimension labels (", ndim, ")"));
    }
  };
  check_ndim(values, "values");
  check_ndim(variances, "variances");
  if (!shape.is_none() && scipp::index(py::len(shape)) != ndim) {
    throw except::DimensionError(
        format("The number of dimensions in 'shape' (", py::len(shape),
               ") does not match the number of dimension labels (", ndim, ")"));
  }
}

Dimensions build_dimensions(const std::vector<Dim> &dim_labels,
                            const py::object &shape,
                            const std::optional<py::array> &values,
                            const std::optional<py::array> &variances) {
  ensure_consistent_ndim(dim_labels, shape, values, variances);
  const auto ndim = scipp::size(dim_labels);

  // We cannot easily index into shape. Store it to compare to the data later.
  const bool got_shape = !shape.is_none();
  auto deduced_shape = got_shape ? shape.cast<std::vector<scipp::index>>()
                                 : std::vector<scipp::index>(ndim);

  for (scipp::index d = 0; d < ndim; ++d) {
    scipp::index size = -1;
    if (values.has_value()) {
      if (variances.has_value() && values->shape(d) != variances->shape(d)) {
        throw except::DimensionError(
            format("The shapes of values and variances differ in dimension ", d,
                   ": ", py::object(values->attr("shape")), " vs ",
                   py::object(variances->attr("shape")), '.'));
      }
      size = values->shape(d);
    } else {
      if (variances.has_value()) {
        size = variances->shape(d);
      }
    }
    if (got_shape && deduced_shape[d] != size) {
      throw except::DimensionError(format(
          "The shape of the data differs from the given shape in dimension ", d,
          ": ", size, " vs ", deduced_shape[d], '.'));
    } else {
      deduced_shape[d] = size;
    }
  }

  return Dimensions{dim_labels, deduced_shape};
}

void ensure_is_scalar(const py::buffer &array) {
  if (const auto ndim = array.attr("ndim").cast<int64_t>(); ndim != 0) {
    throw except::DimensionError(
        format("Cannot interpret ", ndim, "-dimensional array as a scalar."));
  }
}

template <class T>
T extract_scalar(const py::object &obj, const units::Unit unit) {
  using TM = ElementTypeMap<T>;
  using PyType = typename TM::PyType;
  TM::check_assignable(obj, unit);
  if (py::isinstance<py::buffer>(obj)) {
    ensure_is_scalar(obj);
    return converting_cast<PyType>::cast(obj.attr("item")());
  } else {
    return converting_cast<PyType>::cast(obj);
  }
}

template <>
core::time_point extract_scalar<core::time_point>(const py::object &obj,
                                                  const units::Unit unit) {
  using TM = ElementTypeMap<core::time_point>;
  using PyType = typename TM::PyType;
  TM::check_assignable(obj, unit);
  if (py::isinstance<py::buffer>(obj)) {
    ensure_is_scalar(obj);
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
  using TM = ElementTypeMap<python::PyObject>;
  TM::check_assignable(obj, unit);
  return obj;
}

template <class T> struct MakeVariableDefaultInit {
  static Variable apply(const Dimensions &dims, const units::Unit unit,
                        const std::optional<bool> with_variance) {
    auto var = with_variance.value_or(false)
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

template <class T> struct MakeVariableScalar {
  static Variable apply(const py::object &value, const py::object &variance,
                        units::Unit unit,
                        const std::optional<bool> with_variance) {
    auto variable = [&]() {
      auto val = [&]() {
        if (value.is_none()) {
          return element_array<T>();
        } else {
          const auto [actual_unit, conversion_factor] =
              common_unit<T>(value, unit);
          unit = actual_unit;
          if (conversion_factor != 1) {
            // TODO Triggered once common_unit implements conversions.
            std::terminate();
          }
          return element_array<T>(1, extract_scalar<T>(value, unit));
        }
      }();
      auto var = [&]() {
        if (variance.is_none()) {
          return element_array<T>();
        } else {
          // No need to call common_unit as of now as it only matters for
          // datetimes which cannot have variances.
          return element_array<T>(1, extract_scalar<T>(variance, unit));
        }
      }();
      if ((with_variance.has_value() && *with_variance) ||
          (!with_variance.has_value() && var)) {
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

template <class T> struct MakeVariableArray {
  static Variable apply(const Dimensions &dims, const py::object &values,
                        const py::object &variances, const units::Unit unit,
                        const std::optional<bool> with_variance) {
    auto variable =
        ((with_variance.has_value() && *with_variance) ||
         (!with_variance.has_value() && !variances.is_none()))
            ? makeVariable<T>(
                  dims, Values(dims.volume(), core::default_init_elements),
                  Variances(dims.volume(), core::default_init_elements))
            : makeVariable<T>(
                  dims, Values(dims.volume(), core::default_init_elements));

    const auto [actual_unit, conversion_factor] = common_unit<T>(values, unit);
    variable.setUnit(actual_unit);
    if (conversion_factor != 1) {
      // TODO Triggered once common_unit implements conversions.
      std::terminate();
    }

    if (!values.is_none()) {
      copy_array_into_view(cast_to_array_like<T>(values, actual_unit),
                           variable.template values<T>(), dims);
    }
    if (with_variance.value_or(true) && !variances.is_none()) {
      copy_array_into_view(cast_to_array_like<T>(variances, actual_unit),
                           variable.template variances<T>(), dims);
    }
    return variable;
  }
};

Variable make_variable_array(const py::object &dim_labels,
                             const py::object &shape, const py::object &values,
                             const py::object &variances,
                             const units::Unit unit, DType dtype,
                             const std::optional<bool> with_variance) {
  using opt_array = std::optional<py::array>;
  // Let numpy figure out conversions from lists, tuples, etc.
  const opt_array values_array{values.is_none() ? opt_array{}
                                                : py::array(values)};
  const opt_array variances_array{variances.is_none() ? opt_array{}
                                                      : py::array(variances)};
  dtype = common_dtype(
      values_array ? py::object(*values_array) : py::none(),
      variances_array ? py::object(*variances_array) : py::none(), dtype, true);
  const auto dims = build_dimensions(dim_labels.cast<std::vector<Dim>>(), shape,
                                     values_array, variances_array);
  return core::CallDType<
      double, float, int64_t, int32_t, bool, scipp::core::time_point,
      std::string, python::PyObject>::apply<MakeVariableArray>(dtype, dims,
                                                               values,
                                                               variances, unit,
                                                               with_variance);
}

std::string make_scalar_array_conflict_message(const py::object &values,
                                               const py::object &variances,
                                               const py::object &value,
                                               const py::object &variance) {
  std::ostringstream oss;
  oss << "Scalar and array arguments cannot both be used to initialize "
         "variables. Got ";
  bool first = true;
  if (!values.is_none()) {
    oss << "'values'";
    first = false;
  }
  if (!variances.is_none()) {
    if (!first)
      oss << " and ";
    oss << "'variances'";
  }
  oss << " which cannot be combined with ";
  first = true;
  if (!value.is_none()) {
    oss << "'value'";
    first = false;
  }
  if (!variance.is_none()) {
    if (!first)
      oss << " and ";
    oss << "'variance'";
  }
  oss << '.';
  return oss.str();
}

enum class ConstructorType { Array, Scalar, Default };

ConstructorType
identify_constructor(const py::object &dims, const py::object &shape,
                     const py::object &values, const py::object &variances,
                     const py::object &value, const py::object &variance) {
  if (!values.is_none() || !variances.is_none()) {
    if (!value.is_none() || !variance.is_none()) {
      throw std::invalid_argument(make_scalar_array_conflict_message(
          values, variances, value, variance));
    }
    if (dims.is_none()) {
      throw std::invalid_argument(
          "Array constructor of variable missing dims argument. "
          "Constructor type deduced from presence of values / variances "
          "arguments.");
    }
    return ConstructorType::Array;
  } else if (!value.is_none() || !variance.is_none()) {
    if (!dims.is_none() && py::len(dims) != 0) {
      throw std::invalid_argument(
          format("Too many dims for constructing a scalar: ", dims,
                 ". Constructor type deduced from presence of value / "
                 "variance arguments."));
    }
    if (!shape.is_none() && py::len(shape) != 0) {
      throw std::invalid_argument(format(
          "Too many dimensions in shape for constructing a scalar: ", shape,
          ". Constructor type deduced from presence of value / "
          "variance arguments."));
    }
    return ConstructorType::Scalar;
  } else {
    return ConstructorType::Default;
  }
}
} // namespace

void bind_init(py::class_<Variable> &cls) {
  cls.def(
      py::init([](const py::object &dims, const py::object &shape,
                  const py::object &values, const py::object &variances,
                  const py::object &value, const py::object &variance,
                  const std::optional<bool> with_variance,
                  const std::optional<units::Unit> unit,
                  const py::object &dtype) {
        const auto [scipp_dtype, actual_unit] =
            cast_dtype_and_unit(dtype, unit);

        switch (identify_constructor(dims, shape, values, variances, value,
                                     variance)) {
        case ConstructorType::Array:
          return make_variable_array(dims, shape, values, variances,
                                     actual_unit, scipp_dtype, with_variance);
        case ConstructorType::Scalar:
          return make_variable_scalar(value, variance, actual_unit, scipp_dtype,
                                      with_variance);
        case ConstructorType::Default:
          return make_variable_default_init(dims, shape, actual_unit,
                                            scipp_dtype, with_variance);
        }
        // Unreachable but gcc complains about a missing return otherwise.
        std::terminate();
      }),
      py::kw_only(), py::arg("dims") = py::none(),
      py::arg("shape") = py::none(), py::arg("values") = py::none(),
      py::arg("variances") = py::none(), py::arg("value") = py::none(),
      py::arg("variance") = py::none(), py::arg("with_variance") = std::nullopt,
      py::arg("unit") = std::nullopt, py::arg("dtype") = py::none(),
      R"raw(
Initialize a variable.

Constructing variables can be tricky because there are many arguments, some of
which are mutually exclusive or require certain (combinations of) other
arguments. Because of this, it is recommended to use the dedicated creation
functions :py:func:`scipp.array` and :py:func:`scipp.scalar`.

Argument dependencies:

- Array variable: ``dims``, and at least one of ``shape``, ``values``,
  ``variances`` are required. But ``value`` and ``variance`` are not allowed.
- Scalar variable: ``dims`` and ``shape`` must be absent or empty.
  ``value`` and ``variance`` are used to optionally set initial values.
  ``values`` and ``variances`` are not allowed.

:param dims: Dimension labels.
:param shape: Size in each dimension.
:param values: Sequence of values for constructing an array variable.
:param variances: Sequence of variances for constructing an array variable.
:param value: A single value for constructing a scalar variable.
:param variance: A single variance for constructing a scalar variable.
:param with_variance: - If True, either store variance(s) given by args
                        ``variances`` or ``variance`` or if those are None,
                        create default initialized variances.
                      - If False, no variances are stored even if ``variances``
                        or ``variance`` are given.
                      - If left unspecified, the arguments ``variances`` and
                        ``variance`` control whether the resulting variable has
                        any variances.
:param unit: Physical unit, defaults to ``scipp.units.dimensionless``.
:param dtype: Type of the variable's elements. Is deduced from other arguments
              in most cases. Defaults to ``sc.dtype.float64`` if no deduction is
              possible.

:type dims: Sequence[str]
:type shape: Sequence[int]
:type values: numpy.ArrayLike
:type variances: numpy.ArrayLike
:type value: Any
:type variance: Any
:type with_variance: bool
:type unit: scipp.Unit
:type dtype: Any
)raw");
}