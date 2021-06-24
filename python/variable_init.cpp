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

auto len_or_0(const py::object &sequence) {
  return sequence.is_none() ? 0 : py::len(sequence);
}

// TODO merge build_dimensions?
Dimensions build_dimensions(const py::object &labels, const py::object &shape) {
  const auto n_labels = len_or_0(labels);
  const auto n_shape = len_or_0(shape);
  if (n_labels != n_shape) {
    throw std::invalid_argument(
        format("Inconsistent dimension labels and shape. Got ", n_labels,
               " labels and ", n_shape, " sizes."));
  } else if (n_labels == 0) {
    return Dimensions{};
  } else {
    return Dimensions(labels.cast<std::vector<Dim>>(),
                      shape.cast<std::vector<scipp::index>>());
  }
}

scipp::index get_ndim(const std::vector<Dim> &dim_labels,
                      const py::object &shape,
                      const std::optional<py::array> &values,
                      const std::optional<py::array> &variances) {
  const auto ndim = scipp::size(dim_labels);
  if (!shape.is_none() && scipp::index(py::len(shape)) != ndim) {
    throw except::DimensionError(
        format("The number of dimensions in 'shape' (", py::len(shape),
               ") does not match the number of dimension labels (", ndim, ")"));
  }

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

  return ndim;
}

Dimensions build_dimensions(const std::vector<Dim> &dim_labels,
                            const py::object &shape,
                            const std::optional<py::array> &values,
                            const std::optional<py::array> &variances) {
  const auto ndim = get_ndim(dim_labels, shape, values, variances);

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

template <class T>
auto make_element_array(const Dimensions &dims, const py::object &source,
                        const units::Unit unit) {
  if (source.is_none()) {
    return element_array<T>();
  } else if (dims.ndim() == 0) {
    return element_array<T>(1, extract_scalar<T>(source, unit));
  } else {
    element_array<T> array(dims.volume(), core::default_init_elements);
    copy_array_into_view(cast_to_array_like<T>(source, unit), array, dims);
    return array;
  }
}

template <class T> struct MakeVariableWithData {
  static Variable apply(const Dimensions &dims, const py::object &values,
                        const py::object &variances, const units::Unit unit,
                        const std::optional<bool> with_variance) {
    const auto [actual_unit, conversion_factor] = common_unit<T>(values, unit);
    if (conversion_factor != 1) {
      // TODO Triggered once common_unit implements conversions.
      std::terminate();
    }

    auto values_array =
        Values(make_element_array<T>(dims, values, actual_unit));
    const bool use_variances =
        (with_variance.has_value() && *with_variance) || !variances.is_none();
    auto variable = use_variances
                        ? makeVariable<T>(dims, std::move(values_array),
                                          Variances(make_element_array<T>(
                                              dims, variances, actual_unit)))
                        : makeVariable<T>(dims, std::move(values_array));
    variable.setUnit(actual_unit);
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
      python::PyObject>::apply<MakeVariableWithData>(dtype, Dimensions{}, value,
                                                     variance, unit,
                                                     with_variance);
}

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
      std::string,
      python::PyObject>::apply<MakeVariableWithData>(dtype, dims, values,
                                                     variances, unit,
                                                     with_variance);
}

bool is_arg_present(const py::object &arg) { return !arg.is_none(); }

template <class T> bool is_arg_present(const std::optional<T> &arg) {
  return arg.has_value();
}

template <class A, class B>
void ensure_mutual_exclusivity(const A &a, const std::string_view a_name,
                               const B &b, const std::string_view b_name) {
  if (is_arg_present(a) && is_arg_present(b)) {
    throw std::invalid_argument(format("Passed mutually exclusive arguments '",
                                       a_name, "' and '", b_name, "'."));
  }
}

enum class ConstructorType { Array, Scalar, Default };

ConstructorType identify_constructor(const py::object &dim_labels,
                                     const py::object &values,
                                     const py::object &variances) {
  if (values.is_none() && variances.is_none()) {
    return ConstructorType::Default;
  } else if (py::bool_{dim_labels}) {
    return ConstructorType::Array;
  } else {
    return ConstructorType::Scalar;
  }
}
} // namespace

/*
 * It is the init method's responsibility to check that the combination
 * of arguments is valid. Functions down the line do not check again.
 */
void bind_init(py::class_<Variable> &cls) {
  cls.def(py::init([](const py::object &dim_labels, const py::object &shape,
                      const py::object &values, const py::object &variances,
                      const std::optional<bool> with_variance,
                      const std::optional<units::Unit> unit,
                      const py::object &dtype) {
            ensure_mutual_exclusivity(variances, "variances", with_variance,
                                      "with_variance");
            ensure_mutual_exclusivity(shape, "shape", values, "values");
            ensure_mutual_exclusivity(shape, "shape", variances, "variances");

            const auto [scipp_dtype, actual_unit] =
                cast_dtype_and_unit(dtype, unit);

            switch (identify_constructor(dim_labels, values, variances)) {
            case ConstructorType::Array:
              return make_variable_array(dim_labels, shape, values, variances,
                                         actual_unit, scipp_dtype,
                                         with_variance);
            case ConstructorType::Scalar:
              return make_variable_scalar(values, variances, actual_unit,
                                          scipp_dtype, with_variance);
            case ConstructorType::Default:
              return make_variable_default_init(dim_labels, shape, actual_unit,
                                                scipp_dtype, with_variance);
            }
            // Unreachable but gcc complains about a missing return otherwise.
            std::terminate();
          }),
          py::kw_only(), py::arg("dims"), py::arg("shape") = py::none(),
          py::arg("values") = py::none(), py::arg("variances") = py::none(),
          py::arg("with_variance") = std::nullopt,
          py::arg("unit") = std::nullopt, py::arg("dtype") = py::none(),
          // TODO fix docs
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