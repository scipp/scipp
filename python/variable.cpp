// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/units/unit.h"

#include "scipp/common/numeric.h"

#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/core/tag_util.h"

#include "scipp/variable/comparison.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/sort.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "docstring.h"
#include "dtype.h"
#include "make_variable.h"
#include "numpy.h"
#include "py_object.h"
#include "pybind11.h"
#include "rename.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> void bind_init_0D(py::class_<Variable> &c) {
  c.def(py::init([](const T &value, const std::optional<T> &variance,
                    const units::Unit &unit) {
          return do_init_0D(value, variance, unit);
        }),
        py::arg("value"), py::arg("variance") = std::nullopt,
        py::arg("unit") = units::one);
}

// This function is used only to bind native python types: pyInt -> int64_t;
// pyFloat -> double; pyBool->bool
template <class T>
void bind_init_0D_native_python_types(py::class_<Variable> &c) {
  c.def(py::init([](const T &value, const std::optional<T> &variance,
                    const units::Unit &unit, py::object &dtype) {
          static_assert(std::is_same_v<T, int64_t> ||
                        std::is_same_v<T, double> || std::is_same_v<T, bool>);
          if (dtype.is_none())
            return do_init_0D(value, variance, unit);
          else {
            return MakeODFromNativePythonTypes<T>::make(unit, value, variance,
                                                        dtype);
          }
        }),
        py::arg("value"), py::arg("variance") = std::nullopt,
        py::arg("unit") = units::one, py::arg("dtype") = py::none());
}

void bind_init_0D_numpy_types(py::class_<Variable> &c) {
  c.def(py::init([](py::buffer &b, const std::optional<py::buffer> &v,
                    const units::Unit &unit, py::object &dtype) {
          py::buffer_info info = b.request();
          if (info.ndim == 0) {
            auto arr = py::array(b);
            auto varr = v ? std::optional{py::array(*v)} : std::nullopt;
            return doMakeVariable({}, arr, varr, unit, dtype);
          } else if (info.ndim == 1 &&
                     scipp_dtype(dtype) == core::dtype<Eigen::Vector3d>) {
            return do_init_0D<Eigen::Vector3d>(
                b.cast<Eigen::Vector3d>(),
                v ? std::optional(v->cast<Eigen::Vector3d>()) : std::nullopt,
                unit);
          } else if (info.ndim == 2 &&
                     scipp_dtype(dtype) == core::dtype<Eigen::Matrix3d>) {
            return do_init_0D<Eigen::Matrix3d>(
                b.cast<Eigen::Matrix3d>(),
                v ? std::optional(v->cast<Eigen::Matrix3d>()) : std::nullopt,
                unit);
          } else {
            throw scipp::except::VariableError(
                "Wrong overload for making 0D variable.");
          }
        }),
        py::arg("value").noconvert(), py::arg("variance") = std::nullopt,
        py::arg("unit") = units::one, py::arg("dtype") = py::none());
}

void bind_init_list(py::class_<Variable> &c) {
  c.def(py::init([](const std::array<Dim, 1> &label, const py::list &values,
                    const std::optional<py::list> &variances,
                    const units::Unit &unit, py::object &dtype) {
          auto arr = py::array(values);
          auto varr =
              variances ? std::optional(py::array(*variances)) : std::nullopt;
          auto dims = std::vector<Dim>{label[0]};
          return doMakeVariable(dims, arr, varr, unit, dtype);
        }),
        py::arg("dims"), py::arg("values"), py::arg("variances") = std::nullopt,
        py::arg("unit") = units::one, py::arg("dtype") = py::none());
}

void bind_init_0D_list_eigen(py::class_<Variable> &c) {
  c.def(
      py::init([](const py::list &value,
                  const std::optional<py::list> &variance,
                  const units::Unit &unit, py::object &dtype) {
        if (scipp_dtype(dtype) == core::dtype<Eigen::Vector3d>) {
          return do_init_0D<Eigen::Vector3d>(
              Eigen::Vector3d(value.cast<std::vector<double>>().data()),
              variance ? std::optional(variance->cast<Eigen::Vector3d>())
                       : std::nullopt,
              unit);
        } else {
          throw scipp::except::VariableError(
              "Cannot create 0D Variable from list of values with this dtype.");
        }
      }),
      py::arg("value"), py::arg("variance") = std::nullopt,
      py::arg("unit") = units::one, py::arg("dtype") = py::none());
}

template <class T, class... Ignored>
void bind_astype(py::class_<T, Ignored...> &c) {
  c.def(
      "astype",
      [](const T &self, const DType type) { return astype(self, type); },
      py::call_guard<py::gil_scoped_release>(),
      R"(
        Converts a Variable to a different type.

        :raises: If the variable cannot be converted to the requested dtype.
        :return: New Variable with specified dtype.
        :rtype: Variable)");
}

void init_variable(py::module &m) {
  py::class_<Variable> variable(m, "Variable",
                                R"(
Array of values with dimension labels and a unit, optionally including an array
of variances.)");
  bind_init_0D<DataArray>(variable);
  bind_init_0D<Dataset>(variable);
  bind_init_0D<std::string>(variable);
  bind_init_0D<Eigen::Vector3d>(variable);
  bind_init_0D<Eigen::Matrix3d>(variable);
  variable.def(py::init<const VariableView &>())
      .def(py::init(&makeVariableDefaultInit),
           py::arg("dims") = std::vector<Dim>{},
           py::arg("shape") = std::vector<scipp::index>{},
           py::arg("unit") = units::one,
           py::arg("dtype") = py::dtype::of<double>(),
           py::arg("variances").noconvert() = false)
      .def(py::init(&doMakeVariable), py::arg("dims"),
           py::arg("values"), // py::array
           py::arg("variances") = std::nullopt, py::arg("unit") = units::one,
           py::arg("dtype") = py::none())
      .def("rename_dims", &rename_dims<Variable>, py::arg("dims_dict"),
           "Rename dimensions.")
      .def(
          "copy", [](const Variable &self) { return self; },
          py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.")
      .def(
          "__copy__", [](Variable &self) { return Variable(self); },
          py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.")
      .def(
          "__deepcopy__",
          [](Variable &self, py::dict) { return Variable(self); },
          py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.")
      .def_property_readonly("dtype", &Variable::dtype)
      .def(
          "__radd__", [](Variable &a, double &b) { return a + b * units::one; },
          py::is_operator())
      .def(
          "__radd__", [](Variable &a, int &b) { return a + b * units::one; },
          py::is_operator())
      .def(
          "__rsub__", [](Variable &a, double &b) { return b * units::one - a; },
          py::is_operator())
      .def(
          "__rsub__", [](Variable &a, int &b) { return b * units::one - a; },
          py::is_operator())
      .def(
          "__rmul__",
          [](Variable &a, double &b) { return a * (b * units::one); },
          py::is_operator())
      .def(
          "__rmul__", [](Variable &a, int &b) { return a * (b * units::one); },
          py::is_operator())
      .def(
          "__rtruediv__",
          [](Variable &a, double &b) { return (b * units::one) / a; },
          py::is_operator())
      .def(
          "__rtruediv__",
          [](Variable &a, int &b) { return (b * units::one) / a; },
          py::is_operator())
      .def("__repr__", [](const Variable &self) { return to_string(self); });

  bind_init_list(variable);
  // Order matters for pybind11's overload resolution. Do not change.
  bind_init_0D_numpy_types(variable);
  bind_init_0D_native_python_types<bool>(variable);
  bind_init_0D_native_python_types<int64_t>(variable);
  bind_init_0D_native_python_types<double>(variable);
  bind_init_0D<py::object>(variable);
  bind_init_0D_list_eigen(variable);
  //------------------------------------

  py::class_<VariableConstView>(m, "VariableConstView")
      .def(py::init<const Variable &>())
      .def(
          "copy", [](const VariableConstView &self) { return Variable(self); },
          "Return a (deep) copy.")
      .def("__copy__",
           [](const VariableConstView &self) { return Variable(self); })
      .def("__deepcopy__",
           [](VariableView &self, py::dict) { return Variable(self); })
      .def("__repr__",
           [](const VariableConstView &self) { return to_string(self); });

  py::class_<VariableView, VariableConstView> variableView(
      m, "VariableView", py::buffer_protocol(), R"(
View for Variable, representing a sliced or transposed view onto a variable;
Mostly equivalent to Variable, see there for details.)");
  variableView.def_buffer(&make_py_buffer_info);
  variableView.def(py::init<Variable &>())
      .def(
          "__radd__",
          [](VariableView &a, double &b) { return a + b * units::one; },
          py::is_operator())
      .def(
          "__rsub__",
          [](VariableView &a, double &b) { return b * units::one - a; },
          py::is_operator())
      .def(
          "__rmul__",
          [](VariableView &a, double &b) { return a * (b * units::one); },
          py::is_operator());

  bind_astype(variable);
  bind_astype(variableView);

  bind_slice_methods(variable);
  bind_slice_methods(variableView);

  bind_comparison<Variable>(variable);
  bind_comparison<VariableConstView>(variable);
  bind_comparison<Variable>(variableView);
  bind_comparison<VariableConstView>(variableView);

  bind_in_place_binary<Variable>(variable);
  bind_in_place_binary<VariableConstView>(variable);
  bind_in_place_binary<Variable>(variableView);
  bind_in_place_binary<VariableConstView>(variableView);
  bind_in_place_binary_scalars(variable);
  bind_in_place_binary_scalars(variableView);

  bind_binary<Variable>(variable);
  bind_binary<VariableConstView>(variable);
  bind_binary<DataArrayView>(variable);
  bind_binary<Variable>(variableView);
  bind_binary<VariableConstView>(variableView);
  bind_binary<DataArrayView>(variableView);
  bind_binary_scalars(variable);
  bind_binary_scalars(variableView);

  bind_boolean_unary(variable);
  bind_boolean_unary(variableView);
  bind_boolean_operators<Variable>(variable);
  bind_boolean_operators<VariableConstView>(variable);
  bind_boolean_operators<Variable>(variableView);
  bind_boolean_operators<VariableConstView>(variableView);

  bind_data_properties(variable);
  bind_data_properties(variableView);

  py::implicitly_convertible<std::string, Dim>();
  py::implicitly_convertible<Variable, VariableConstView>();
  py::implicitly_convertible<Variable, VariableView>();

  m.def(
      "reshape",
      [](const VariableView &self, const std::vector<Dim> &labels,
         const py::tuple &shape) {
        Dimensions dims(labels, shape.cast<std::vector<scipp::index>>());
        return self.reshape(dims);
      },
      py::arg("x"), py::arg("dims"), py::arg("shape"),
      Docstring()
          .description("Reshape a variable.")
          .raises("If the volume of the old shape is not equal to the volume "
                  "of the new shape.")
          .returns("New variable with requested dimension labels and shape.")
          .rtype("Variable")
          .param("x", "Variable to reshape.", "Variable.")
          .param("dims", "List of new dimensions.", "list")
          .param("shape", "New extents in each dimension.", "list")
          .c_str());

  m.def(
      "filter", py::overload_cast<const Variable &, const Variable &>(&filter),
      py::arg("x"), py::arg("filter"), py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description(
              "Selects elements for a Variable using a filter (mask).\n\n"
              "The filter variable must be 1D and of bool type. "
              "A true value in the filter means the corresponding element in "
              "the input is "
              "selected and will be copied to the output. "
              "A false value in the filter discards the corresponding element "
              "in the input.")
          .raises("If the filter variable is not 1 dimensional.")
          .returns("New variable containing the data selected by the filter.")
          .rtype("Variable")
          .param("x", "Variable to filter.", "Variable.")
          .param("filter", "Variable which defines the filter.", "Variable.")
          .c_str());

  m.def("split",
        py::overload_cast<const Variable &, const Dim,
                          const std::vector<scipp::index> &>(&split),
        py::call_guard<py::gil_scoped_release>(),
        "Split a Variable along a given Dimension.");

  m.def(
      "is_linspace",
      [](const VariableConstView &x) {
        if (x.dims().ndim() != 1)
          throw scipp::except::VariableError(
              "is_linspace can only be called on a 1D Variable.");
        else
          return scipp::numeric::is_linspace(x.template values<double>());
      },
      py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("Check if the values of a variable are evenly spaced.")
          .returns("Returns True if the variable contains regularly spaced "
                   "values, False otherwise.")
          .rtype("bool")
          .c_str());
}
