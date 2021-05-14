// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/units/unit.h"

#include "scipp/common/numeric.h"

#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/core/time_point.h"

#include "scipp/variable/operations.h"
#include "scipp/variable/rebin.h"
#include "scipp/variable/structures.h"
#include "scipp/variable/variable.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/util.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "docstring.h"
#include "dtype.h"
#include "make_variable.h"
#include "numpy.h"
#include "pybind11.h"
#include "rename.h"
#include "unit.h"

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
  if constexpr (std::is_same_v<T, Variable> || std::is_same_v<T, DataArray> ||
                std::is_same_v<T, Dataset>) {
    c.def(py::init([](const T &value, const std::optional<T> &variance,
                      const units::Unit &unit) {
            return do_init_0D(copy(value), variance, unit);
          }),
          py::arg("value"), py::arg("variance") = std::nullopt,
          py::arg("unit") = units::one);
  }
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
        py::arg("value").noconvert(), py::arg("variance") = std::nullopt,
        py::arg("unit") = units::one, py::arg("dtype") = py::none());
}

void bind_init_0D_numpy_types(py::class_<Variable> &c) {
  c.def(py::init([](py::buffer &b, const std::optional<py::buffer> &v,
                    const units::Unit &unit, py::object &dtype) {
          static auto np_datetime64_type =
              py::module::import("numpy").attr("datetime64").get_type();

          py::buffer_info info = b.request();
          if (info.ndim == 0) {
            auto arr = py::array(b);
            auto varr = v ? std::optional{py::array(*v)} : std::nullopt;
            return doMakeVariable({}, arr, varr, unit, dtype);
          } else if ((info.ndim == 1) &&
                     py::isinstance(b.get_type(), np_datetime64_type)) {
            if (v.has_value()) {
              throw except::VariancesError("datetimes cannot have variances.");
            }
            const auto [actual_unit, value_factor] =
                get_time_unit(b, dtype, unit);
            return do_init_0D<core::time_point>(
                make_time_point(b, value_factor), std::nullopt, actual_unit);

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

template <class T, class Elem, int... N>
void bind_structured_creation(py::module &m, const std::string &name) {
  m.def(
      name.c_str(),
      [](const std::vector<Dim> &labels, py::array_t<Elem> &values,
         units::Unit unit) {
        if (scipp::size(labels) != values.ndim() - scipp::index(sizeof...(N)))
          throw std::runtime_error("bad shape to make structured type");
        auto var = variable::make_structures<T, Elem, N...>(
            Dimensions(labels,
                       std::vector<scipp::index>(
                           values.shape(), values.shape() + labels.size())),
            unit,
            element_array<Elem>(values.size(), core::default_init_elements));
        auto elems = var.template elements<T>();
        copy_array_into_view(values, elems.template values<Elem>(),
                             elems.dims());
        return var;
      },
      py::arg("dims"), py::arg("values"), py::arg("unit") = units::one);
}

void require(const Variable &var, Eigen::Vector3d) {
  if (var.dtype() != dtype<Eigen::Vector3d>)
    throw except::TypeError(
        "Vector element access properties `x1`, `x2`, and `x3` not "
        "supported for dtype=" +
        to_string(var.dtype()));
}

void require(const Variable &var, Eigen::Matrix3d) {
  if (var.dtype() != dtype<Eigen::Matrix3d>)
    throw except::TypeError(
        "Matrix element access properties `x11`, `x12`, ... not "
        "supported for dtype=" +
        to_string(var.dtype()));
}

template <class T, scipp::index... I>
void bind_elem_property(py::class_<Variable> &v, const char *name) {
  v.def_property(
      name,
      [](Variable &self) {
        require(self, T{});
        return self.elements<T>(I...);
      },
      [](Variable &self, const Variable &elems) {
        require(self, T{});
        copy(elems, self.elements<T>(I...));
      });
}

void init_variable(py::module &m) {
  // Needed to let numpy arrays keep alive the scipp buffers.
  // VariableConcept must ALWAYS be passed to Python by its handle.
  py::class_<VariableConcept, VariableConceptHandle> variable_concept(
      m, "_VariableConcept");

  py::class_<Variable> variable(m, "Variable", py::dynamic_attr(),
                                R"(
Array of values with dimension labels and a unit, optionally including an array
of variances.)");
  bind_init_0D<Variable>(variable);
  bind_init_0D<DataArray>(variable);
  bind_init_0D<Dataset>(variable);
  bind_init_0D<std::string>(variable);
  variable
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
      .def("__sizeof__",
           [](const Variable &self) {
             return size_of(self, SizeofTag::ViewOnly);
           })
      .def("underlying_size", [](const Variable &self) {
        return size_of(self, SizeofTag::Underlying);
      });

  bind_init_list(variable);
  // Order matters for pybind11's overload resolution. Do not change.
  bind_init_0D_numpy_types(variable);
  bind_init_0D_native_python_types<bool>(variable);
  bind_init_0D_native_python_types<int64_t>(variable);
  bind_init_0D_native_python_types<double>(variable);
  bind_init_0D<py::object>(variable);
  //------------------------------------

  bind_common_operators(variable);

  bind_astype(variable);

  bind_slice_methods(variable);

  bind_comparison<Variable>(variable);

  bind_in_place_binary<Variable>(variable);
  bind_in_place_binary_scalars(variable);

  bind_binary<Variable>(variable);
  bind_binary<DataArray>(variable);
  bind_binary_scalars(variable);

  bind_unary(variable);

  bind_boolean_unary(variable);
  bind_logical<Variable>(variable);

  bind_data_properties(variable);

  py::implicitly_convertible<std::string, Dim>();

  m.def(
      "islinspace",
      [](const Variable &x) {
        if (x.dims().ndim() != 1)
          throw scipp::except::VariableError(
              "islinspace can only be called on a 1D Variable.");
        else
          return scipp::numeric::islinspace(x.template values<double>());
      },
      py::call_guard<py::gil_scoped_release>());

  m.def("rebin",
        py::overload_cast<const Variable &, const Dim, const Variable &,
                          const Variable &>(&rebin),
        py::arg("x"), py::arg("dim"), py::arg("old"), py::arg("new"),
        py::call_guard<py::gil_scoped_release>());

  bind_structured_creation<Eigen::Vector3d, double, 3>(m, "vectors");
  bind_structured_creation<Eigen::Matrix3d, double, 3, 3>(m, "matrices");
  bind_elem_property<Eigen::Vector3d, 0>(variable, "x1");
  bind_elem_property<Eigen::Vector3d, 1>(variable, "x2");
  bind_elem_property<Eigen::Vector3d, 2>(variable, "x3");
  bind_elem_property<Eigen::Matrix3d, 0, 0>(variable, "x11");
  bind_elem_property<Eigen::Matrix3d, 0, 1>(variable, "x12");
  bind_elem_property<Eigen::Matrix3d, 0, 2>(variable, "x13");
  bind_elem_property<Eigen::Matrix3d, 1, 0>(variable, "x21");
  bind_elem_property<Eigen::Matrix3d, 1, 1>(variable, "x22");
  bind_elem_property<Eigen::Matrix3d, 1, 2>(variable, "x23");
  bind_elem_property<Eigen::Matrix3d, 2, 0>(variable, "x31");
  bind_elem_property<Eigen::Matrix3d, 2, 1>(variable, "x32");
  bind_elem_property<Eigen::Matrix3d, 2, 2>(variable, "x33");
}
