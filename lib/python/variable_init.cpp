// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "pybind11.h"

#include "scipp/core/dtype.h"
#include "scipp/core/eigen.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/string.h"
#include "scipp/variable/structures.h"
#include "scipp/variable/to_unit.h"
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
bool is_empty(const py::object &sequence) {
  if (py::isinstance<py::buffer>(sequence)) {
    return sequence.attr("ndim").cast<scipp::index>() == 0;
  }
  return !py::bool_{sequence};
}

auto shape_of(const py::object &array) { return py::iter(array.attr("shape")); }

scipp::index n_remaining(const py::iterator &it) {
  return std::distance(it, it.end());
}

[[noreturn]] void throw_ndim_mismatch_error(const scipp::index a_ndim,
                                            const std::string_view a_name,
                                            const scipp::index b_ndim,
                                            const std::string_view b_name) {
  throw std::invalid_argument(
      python::format("The number of dimensions in '", a_name, "' (", a_ndim,
                     ") does not match the number of dimensions in '", b_name,
                     "' (", b_ndim, ")."));
}

void ensure_same_shape(const py::object &values, const py::object &variances) {
  if (values.is_none() || variances.is_none()) {
    return;
  }

  auto val_shape = shape_of(values);
  auto var_shape = shape_of(variances);

  scipp::index dim = 0;
  std::tuple<scipp::index, scipp::index, scipp::index> mismatch{-1, -1, -1};
  for (; val_shape != val_shape.end() && var_shape != var_shape.end();
       ++val_shape, ++var_shape, ++dim) {
    if (val_shape->cast<scipp::index>() != var_shape->cast<scipp::index>()) {
      if (std::get<0>(mismatch) == -1) {
        // Defer throwing to let ndim error take precedence.
        mismatch = std::tuple{dim, val_shape->cast<scipp::index>(),
                              var_shape->cast<scipp::index>()};
      }
    }
  }
  if (val_shape != val_shape.end() || var_shape != var_shape.end()) {
    throw_ndim_mismatch_error(dim + n_remaining(val_shape), "values",
                              dim + n_remaining(var_shape), "variances");
  }
  if (std::get<0>(mismatch) != -1) {
    throw std::invalid_argument(python::format(
        "The shapes of 'values' and 'variances' differ in dimension ",
        std::get<0>(mismatch), ": ", std::get<1>(mismatch), " vs ",
        std::get<2>(mismatch), '.'));
  }
}

namespace detail {
void consume_extra_dims(py::iterator &shape_it,
                        const scipp::index n_extra_dims) {
  for (scipp::index i = 0; i < n_extra_dims; ++i) {
    if (shape_it == shape_it.end())
      throw std::invalid_argument(
          "Data has too few dimensions for given dimension labels.");
    ++shape_it;
  }
}

Dimensions build_dimensions(py::iterator &&label_it, py::iterator &&shape_it,
                            const scipp::index n_extra_dims,
                            const std::string_view shape_name) {
  Dimensions dims;
  scipp::index dim = 0;
  for (; label_it != label_it.end() && shape_it != shape_it.end();
       ++label_it, ++shape_it, ++dim) {
    dims.addInner(Dim{label_it->cast<std::string>()},
                  shape_it->cast<scipp::index>());
  }
  consume_extra_dims(shape_it, n_extra_dims);
  if (label_it != label_it.end() || shape_it != shape_it.end()) {
    throw_ndim_mismatch_error(dim + n_remaining(label_it), "dims",
                              dim + n_remaining(shape_it), shape_name);
  }
  return dims;
}
} // namespace detail

Dimensions build_dimensions(const py::object &dim_labels,
                            const py::object &values,
                            const py::object &variances,
                            const scipp::index n_extra_dims = 0) {
  if (is_empty(dim_labels)) {
    return Dimensions{};
  } else {
    if (!values.is_none()) {
      ensure_same_shape(values, variances);
      return detail::build_dimensions(py::iter(dim_labels), shape_of(values),
                                      n_extra_dims, "values");
    } else {
      return detail::build_dimensions(py::iter(dim_labels), shape_of(variances),
                                      n_extra_dims, "variances");
    }
  }
}

py::object parse_data_sequence(const py::object &dim_labels,
                               const py::object &data) {
  // Need to check for None because py::array does not preserve it.
  if (is_empty(dim_labels) || data.is_none()) {
    return data;
  } else {
    return py::array(data);
  }
}

void ensure_is_scalar(const py::buffer &array) {
  if (const auto ndim = array.attr("ndim").cast<int64_t>(); ndim != 0) {
    throw except::DimensionError(python::format(
        "Cannot interpret ", ndim, "-dimensional array as a scalar."));
  }
}

template <class T>
T extract_scalar(const py::object &obj, const sc_units::Unit unit) {
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
                                                  const sc_units::Unit unit) {
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
                                                  const sc_units::Unit unit) {
  using TM = ElementTypeMap<python::PyObject>;
  TM::check_assignable(obj, unit);
  return obj;
}

template <class T>
auto make_element_array(const Dimensions &dims, const py::object &source,
                        const sc_units::Unit unit) {
  if (source.is_none()) {
    return element_array<T>();
  } else if (dims.ndim() == 0) {
    return element_array<T>(1, extract_scalar<T>(source, unit));
  } else {
    element_array<T> array(dims.volume(), core::init_for_overwrite);
    copy_array_into_view(cast_to_array_like<T>(source, unit), array, dims);
    return array;
  }
}

template <class T> struct MakeVariable {
  static Variable apply(const Dimensions &dims, const py::object &values,
                        const py::object &variances,
                        const sc_units::Unit unit) {
    const auto [values_unit, final_unit] = common_unit<T>(values, unit);
    auto values_array =
        Values(make_element_array<T>(dims, values, values_unit));
    auto variable = variances.is_none()
                        ? makeVariable<T>(dims, std::move(values_array))
                        // cppcheck-suppress accessMoved  # False-positive.
                        : makeVariable<T>(dims, std::move(values_array),
                                          Variances(make_element_array<T>(
                                              dims, variances, values_unit)));
    variable.setUnit(values_unit);
    return to_unit(variable, final_unit, CopyPolicy::TryAvoid);
  }
};

Variable make_variable(const py::object &dim_labels, const py::object &values,
                       const py::object &variances,
                       const std::optional<sc_units::Unit> &unit_,
                       DType dtype) {
  const auto converted_values = parse_data_sequence(dim_labels, values);
  const auto converted_variances = parse_data_sequence(dim_labels, variances);
  dtype = common_dtype(converted_values, converted_variances, dtype);
  const auto dims =
      build_dimensions(dim_labels, converted_values, converted_variances);
  const auto unit = unit_.value_or(variable::default_unit_for(dtype));
  return core::CallDType<double, float, int64_t, int32_t, bool,
                         scipp::core::time_point, std::string, Variable,
                         DataArray, Dataset,
                         python::PyObject>::apply<MakeVariable>(dtype, dims,
                                                                values,
                                                                variances,
                                                                unit);
}

template <int N> Dimensions pad_structure_dimensions(Dimensions dims) {
  dims.addInner(Dim::InternalStructureComponent, N);
  return dims;
}

template <int M, int N> Dimensions pad_structure_dimensions(Dimensions dims) {
  dims.addInner(Dim::InternalStructureRow, M);
  dims.addInner(Dim::InternalStructureColumn, N);
  return dims;
}

template <class T, class Elem, int... N>
Variable make_structured_variable(const py::object &dim_labels,
                                  const py::object &values_,
                                  const py::object &variances,
                                  const std::optional<sc_units::Unit> &unit_) {
  if (!variances.is_none())
    throw except::VariancesError("Variances not supported for dtype " +
                                 to_string(dtype<Elem>));

  const auto values = py::array(values_);
  const auto unit = unit_.value_or(variable::default_unit_for(dtype<Elem>));
  const auto dims =
      build_dimensions(dim_labels, values, py::none(), sizeof...(N));
  const auto padded_dims = pad_structure_dimensions<N...>(dims);

  auto var = variable::make_structures<T, Elem>(
      dims, unit, make_element_array<Elem>(padded_dims, values, unit));
  return var;
}
} // namespace

/*
 * It is the init method's responsibility to check that the combination
 * of arguments is valid. Functions down the line do not check again.
 */
void bind_init(py::class_<Variable> &cls) {
  cls.def(
      py::init([](const py::object &dim_labels, const py::object &values,
                  const py::object &variances, const ProtoUnit unit,
                  const py::object &dtype, const bool aligned) {
        if (values.is_none() && variances.is_none()) {
          throw std::invalid_argument(
              "At least one argument of 'values' and 'variances' is required.");
        }
        const auto [scipp_dtype, actual_unit] =
            cast_dtype_and_unit(dtype, unit);

        auto var = [&, c_scipp_dtype = scipp_dtype,
                    c_actual_unit = actual_unit]() {
          if (c_scipp_dtype == ::dtype<Eigen::Vector3d>)
            return make_structured_variable<Eigen::Vector3d, double, 3>(
                dim_labels, values, variances, c_actual_unit);
          if (c_scipp_dtype == ::dtype<Eigen::Matrix3d>)
            return make_structured_variable<Eigen::Matrix3d, double, 3, 3>(
                dim_labels, values, variances, c_actual_unit);
          if (c_scipp_dtype == ::dtype<Eigen::Affine3d>)
            return make_structured_variable<Eigen::Affine3d, double, 4, 4>(
                dim_labels, values, variances, c_actual_unit);
          if (c_scipp_dtype == ::dtype<core::Quaternion>)
            return make_structured_variable<core::Quaternion, double, 4>(
                dim_labels, values, variances, c_actual_unit);
          if (c_scipp_dtype == ::dtype<core::Translation>)
            return make_structured_variable<core::Translation, double, 3>(
                dim_labels, values, variances, c_actual_unit);

          return make_variable(dim_labels, values, variances, c_actual_unit,
                               c_scipp_dtype);
        }();

        var.set_aligned(aligned);
        return var;
      }),
      py::kw_only(), py::arg("dims"), py::arg("values") = py::none(),
      py::arg("variances") = py::none(), py::arg("unit") = DefaultUnit{},
      py::arg("dtype") = py::none(), py::arg("aligned") = true,
      R"raw(
Initialize a variable with values and/or variances.

At least one argument of ``values`` and ``variances`` must be used.
if you want to preallocate memory to fill later, use :py:func:`scipp.empty`.

Attention
---------
This constructor is meant primarily for internal use.
Use one of the Specialized
`creation functions <https://scipp.github.io/reference/creation-functions.html>`_ instead.
See in particular :py:func:`scipp.array` and :py:func:`scipp.scalar`.

Parameters
----------
dims:
   Dimension labels.
values:
   Sequence of values for constructing an array variable.
variances:
   Sequence of variances for constructing an array variable.
unit:
   Physical unit, defaults to ``scipp.units.dimensionless``.
dtype:
   Type of the variable's elements. Is deduced from other arguments
   in most cases. Defaults to ``sc.DType.float64`` if no deduction is
   possible.
aligned:
   Initial value for the alignment flag.

Examples
--------
Use :py:func:`scipp.array` for 1-D or higher-dimensional data:

  >>> import scipp as sc
  >>> sc.array(dims=['x'], values=[1, 2, 3], unit='m')
  <scipp.Variable> (x: 3)      int64  [m]  [1, 2, 3]

Use :py:func:`scipp.scalar` for 0-D (scalar) data:

  >>> sc.scalar(value=3.14, unit='rad')
  <scipp.Variable> ()    float64  [rad]  3.14
)raw");
}
