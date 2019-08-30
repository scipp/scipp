// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/units/unit.h"

#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/tag_util.h"
#include "scipp/core/variable.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "numpy.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

template <class T> struct MakeVariable {
  static Variable apply(const std::vector<Dim> &labels, py::array values,
                        const std::optional<py::array> &variances,
                        const units::Unit unit) {
    // Pybind11 converts py::array to py::array_t for us, with all sorts of
    // automatic conversions such as integer to double, if required.
    py::array_t<T> valuesT(values);
    py::buffer_info info = valuesT.request();
    Dimensions dims(labels, info.shape);
    auto var =
        variances ? makeVariableWithVariances<T>(dims) : makeVariable<T>(dims);
    copy_flattened<T>(valuesT, var.template values<T>());
    if (variances) {
      py::array_t<T> variancesT(*variances);
      info = variancesT.request();
      expect::equals(dims, Dimensions(labels, info.shape));
      copy_flattened<T>(variancesT, var.template variances<T>());
    }
    var.setUnit(unit);
    return var;
  }
};

template <class T> struct MakeVariableDefaultInit {
  static Variable apply(const std::vector<Dim> &labels,
                        const std::vector<scipp::index> &shape,
                        const units::Unit unit, const bool variances) {
    Dimensions dims(labels, shape);
    auto var = variances ? scipp::core::makeVariableWithVariances<T>(dims)
                         : scipp::core::makeVariable<T>(dims);
    var.setUnit(unit);
    return var;
  }
};

namespace scippy {
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
  if (type.is(py::dtype::of<std::int32_t>()))
    return scipp::core::dtype<int32_t>;
  if (type.is(py::dtype::of<bool>()))
    return scipp::core::dtype<bool>;
  throw std::runtime_error("Unsupported numpy dtype.");
}

// temporary solution untill https://github.com/pybind/pybind11/issues/1538
// is not solved
scipp::core::DType scipp_dtype_fall_back(const py::object &type) {
  py::object numpy = py::module::import("numpy");
  py::object maketype = numpy.attr("dtype");
  py::object new_type = maketype(type);
  return scipp_dtype(new_type.cast<py::dtype>());
}

scipp::core::DType scipp_dtype(const py::object &type) {
  // The manual conversion from py::object is solving a number of problems:
  // 1. On Travis' clang (7.0.0) we get a weird error (ImportError:
  //    UnicodeDecodeError: 'utf-8' codec can't decode byte 0xe1 in position 2:
  //    invalid continuation byte) when using the DType enum as a default value
  //    for py::arg. Importing the module fails.
  // 2. We want to support both numpy dtype as well as scipp dtype.
  // 3. In the implementation below, `type.cast<py::dtype>()` always succeeds,
  //    yielding a unsupported numpy dtype. Therefore we need to try casting to
  //    `DType` first, which works for some reason.
  if (type.is_none())
    return DType::Unknown;
  try {
    return type.cast<DType>();
  } catch (const py::cast_error &) {
    try {
      return scippy::scipp_dtype(type.cast<py::dtype>());
    } catch (...) {
      return scipp_dtype_fall_back(type);
    }
  }
}
} // namespace scippy

Variable doMakeVariable(const std::vector<Dim> &labels, py::array &values,
                        std::optional<py::array> &variances,
                        const units::Unit unit, const py::object &dtype) {
  // Use custom dtype, otherwise dtype of data.
  const auto dtypeTag = dtype.is_none() ? scippy::scipp_dtype(values.dtype())
                                        : scippy::scipp_dtype(dtype);
  return CallDType<double, float, int64_t, int32_t, bool>::apply<MakeVariable>(
      dtypeTag, labels, values, variances, unit);
}

Variable makeVariableDefaultInit(const std::vector<Dim> &labels,
                                 const std::vector<scipp::index> &shape,
                                 const units::Unit unit, py::object &dtype,
                                 const bool variances) {
  return CallDType<double, float, int64_t, int32_t, bool, Dataset,
                   Eigen::Vector3d>::
      apply<MakeVariableDefaultInit>(scippy::scipp_dtype(dtype), labels, shape,
                                     unit, variances);
}

template <class T> void bind_init_0D(py::class_<Variable> &c) {
  c.def(py::init([](const T &value, const std::optional<T> &variance,
                    const units::Unit &unit) {
          Variable var;
          if (variance)
            var = makeVariable<T>(value, *variance);
          else
            var = makeVariable<T>(value);
          var.setUnit(unit);
          return var;
        }),
        py::arg("value"), py::arg("variance") = std::nullopt,
        py::arg("unit") = units::Unit(units::dimensionless));
}

template <class T> void bind_init_1D(py::class_<Variable> &c) {
  c.def(
      // Using fixed-size-1 array for the labels. This avoids the
      // `T=Eigen::Vector3d` overload wrongly matching to an 2d (or higher)
      // array with inner dimension 3 as `values`.
      py::init([](const std::array<Dim, 1> &label, const std::vector<T> &values,
                  const std::optional<std::vector<T>> &variances,
                  const units::Unit &unit) {
        Variable var;
        Dimensions dims({label[0]}, {scipp::size(values)});
        if (variances)
          var = makeVariable<T>(dims, values, *variances);
        else
          var = makeVariable<T>(dims, values);
        var.setUnit(unit);
        return var;
      }),
      py::arg("dims"), py::arg("values"), py::arg("variances") = std::nullopt,
      py::arg("unit") = units::Unit(units::dimensionless));
}

void init_variable(py::module &m) {
  py::class_<Variable> variable(m, "Variable", R"(
    Array of values with dimension labels and a unit, optionally including an array of variances.)");
  bind_init_0D<Dataset>(variable);
  bind_init_0D<int64_t>(variable);
  bind_init_0D<int32_t>(variable);
  bind_init_0D<double>(variable);
  bind_init_0D<float>(variable);
  bind_init_0D<Eigen::Vector3d>(variable);
  bind_init_1D<std::string>(variable);
  bind_init_1D<Eigen::Vector3d>(variable);
  variable.def(py::init<const VariableProxy &>())
      .def(py::init(&makeVariableDefaultInit),
           py::arg("dims") = std::vector<Dim>{},
           py::arg("shape") = std::vector<scipp::index>{},
           py::arg("unit") = units::Unit(units::dimensionless),
           py::arg("dtype") = py::dtype::of<double>(),
           py::arg("variances").noconvert() = false)
      .def(py::init(&doMakeVariable), py::arg("dims"), py::arg("values"),
           py::arg("variances") = std::nullopt,
           py::arg("unit") = units::Unit(units::dimensionless),
           py::arg("dtype") = py::none())
      .def("copy", [](const Variable &self) { return self; },
           "Make a copy of a Variable.")
      .def("__copy__", [](Variable &self) { return Variable(self); })
      .def("__deepcopy__",
           [](Variable &self, py::dict) { return Variable(self); })
      .def_property_readonly("dtype", &Variable::dtype)
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self /= double(), py::call_guard<py::gil_scoped_release>())
      .def("__radd__", [](Variable &a, double &b) { return a + b; },
           py::is_operator())
      .def("__rsub__", [](Variable &a, double &b) { return b - a; },
           py::is_operator())
      .def("__rmul__", [](Variable &a, double &b) { return a * b; },
           py::is_operator())
      .def("__repr__", [](const Variable &self) { return to_string(self); });

  // For some reason, pybind11 does not convert python lists to py::array,
  // so we need to bind the lists manually.
  // TODO: maybe there is a better way to do this?
  bind_init_1D<int32_t>(variable);
  bind_init_1D<double>(variable);

  py::class_<VariableConstProxy>(m, "VariableConstProxy")
      .def(py::init<const Variable &>());
  py::class_<VariableProxy, VariableConstProxy> variableProxy(
      m, "VariableProxy", py::buffer_protocol(), R"(
        Proxy for Variable, representing a sliced or transposed view onto a variable;
        Mostly equivalent to Variable, see there for details.)");
  variableProxy.def_buffer(&make_py_buffer_info);
  variableProxy
      .def("copy", [](const VariableProxy &self) { return Variable(self); },
           "Make a copy of a VariableProxy and return it as a Variable.")
      .def("__copy__", [](VariableProxy &self) { return Variable(self); })
      .def("__deepcopy__",
           [](VariableProxy &self, py::dict) { return Variable(self); })
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self /= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def("__radd__", [](VariableProxy &a, double &b) { return a + b; },
           py::is_operator())
      .def("__rsub__", [](VariableProxy &a, double &b) { return b - a; },
           py::is_operator())
      .def("__rmul__", [](VariableProxy &a, double &b) { return a * b; },
           py::is_operator())
      .def("__repr__",
           [](const VariableProxy &self) { return to_string(self); });

  bind_slice_methods(variable);
  bind_slice_methods(variableProxy);

  bind_comparison<Variable>(variable);
  bind_comparison<VariableProxy>(variable);
  bind_comparison<Variable>(variableProxy);
  bind_comparison<VariableProxy>(variableProxy);

  bind_in_place_binary<Variable>(variable);
  bind_in_place_binary<VariableProxy>(variable);
  bind_in_place_binary<Variable>(variableProxy);
  bind_in_place_binary<VariableProxy>(variableProxy);

  bind_binary<Variable>(variable);
  bind_binary<VariableProxy>(variable);
  bind_binary<Variable>(variableProxy);
  bind_binary<VariableProxy>(variableProxy);

  bind_data_properties(variable);
  bind_data_properties(variableProxy);

  py::implicitly_convertible<Variable, VariableConstProxy>();

  m.def("reshape",
        [](const VariableProxy &self, const std::vector<Dim> &labels,
           const py::tuple &shape) {
          Dimensions dims(labels, shape.cast<std::vector<scipp::index>>());
          return self.reshape(dims);
        },
        py::arg("variable"), py::arg("dims"), py::arg("shape"),
        R"(
        Reshape a variable.

        :raises: If the volume of the old shape is not equal to the volume of the new shape.
        :return: New variable with requested dimension labels and shape.
        :rtype: Variable)");

  m.def("abs", [](const Variable &self) { return abs(self); },
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise absolute value.

        :raises: If the dtype has no absolute value, e.g., if it is a string
        :seealso: :py:class:`scipp.norm` for vector-like dtype
        :return: Copy of the input with values replaced by the absolute values
        :rtype: Variable)");
  m.def("dot", py::overload_cast<const Variable &, const Variable &>(&dot),
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise dot-product.

        :raises: If the dtype is not a vector such as :py:class:`scipp.dtype.vector_3_double`
        :return: New variable with scalar elements based on the two inputs.
        :rtype: Variable)");
  m.def("concatenate",
        py::overload_cast<const Variable &, const Variable &, const Dim>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>(), R"(
        Concatenate input variables along the given dimension.

        Concatenation can happen in two ways:
        - Along an existing dimension, yielding a new dimension extent given by the sum of the input's extents.
        - Along a new dimension that is not contained in either of the inputs, yielding an output with one extra dimensions.

        :raises: If the dtype or unit does not match, or if the dimensions and shapes are incompatible.
        :return: New variable containing all elements of the input variables.
        :rtype: Variable)");
  m.def("filter",
        py::overload_cast<const Variable &, const Variable &>(&filter),
        py::call_guard<py::gil_scoped_release>());
  m.def("mean", py::overload_cast<const Variable &, const Dim>(&mean),
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise mean over the specified dimension.

        :raises: If the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.sum`
        :return: New variable containing the mean.
        :rtype: Variable)");
  m.def("norm", py::overload_cast<const Variable &>(&norm),
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise norm.

        :raises: If the dtype has no norm, i.e., if it is not a vector
        :seealso: :py:class:`scipp.abs` for scalar dtype
        :return: New variable with scalar elements computed as the norm values if the input elements.
        :rtype: Variable)");
  m.def("rebin",
        py::overload_cast<const Variable &, const Variable &, const Variable &>(
            &rebin),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable whose data is rebinned with new bin edges.");
  m.def("split",
        py::overload_cast<const Variable &, const Dim,
                          const std::vector<scipp::index> &>(&split),
        py::call_guard<py::gil_scoped_release>(),
        "Split a Variable along a given Dimension.");

  m.def("sqrt", [](const Variable &self) { return sqrt(self); },
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise square-root.

        :raises: If the dtype has no square-root, e.g., if it is a string
        :return: Copy of the input with values replaced by the square-root.
        :rtype: Variable)");

  m.def("sum", py::overload_cast<const Variable &, const Dim>(&sum),
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise sum over the specified dimension.

        :raises: If the dtype cannot be summed, e.g., if it is a string
        :seealso: :py:class:`scipp.mean`
        :return: New variable containing the sum.
        :rtype: Variable)");

  m.def("sin", [](const Variable &self) { return sin(self); },
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise sin.

        :raises: If the unit is not a plane-angle unit, or if the dtype has no sin, e.g., if it is an integer
        :return: Copy of the input with values replaced by the sin.
        :rtype: Variable)");

  m.def("cos", [](const Variable &self) { return cos(self); },
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise cos.

        :raises: If the unit is not a plane-angle unit, or if the dtype has no cos, e.g., if it is an integer
        :return: Copy of the input with values replaced by the cos.
        :rtype: Variable)");

  m.def("tan", [](const Variable &self) { return tan(self); },
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise tan.

        :raises: If the unit is not a plane-angle unit, or if the dtype has no tan, e.g., if it is an integer
        :return: Copy of the input with values replaced by the tan.
        :rtype: Variable)");

  m.def("asin", [](const Variable &self) { return asin(self); },
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise asin.

        :raises: If the unit is dimensionless, or if the dtype has no asin, e.g., if it is an integer
        :return: Copy of the input with values replaced by the asin. Output unit is rad.
        :rtype: Variable)");

  m.def("acos", [](const Variable &self) { return acos(self); },
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise acos.

        :raises: If the unit is dimensionless, or if the dtype has no acos, e.g., if it is an integer
        :return: Copy of the input with values replaced by the acos. Output unit is rad.
        :rtype: Variable)");

  m.def("atan", [](const Variable &self) { return atan(self); },
        py::call_guard<py::gil_scoped_release>(), R"(
        Element-wise atan.

        :raises: If the unit is dimensionless, or if the dtype has no atan, e.g., if it is an integer
        :return: Copy of the input with values replaced by the atan. Output unit is rad.
        :rtype: Variable)");
}
