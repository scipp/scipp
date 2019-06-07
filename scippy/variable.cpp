// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <variant>

#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "bind_data_access.h"
#include "bind_slice_methods.h"
#include "dataset.h"
#include "except.h"
#include "numpy.h"
#include "scipp/units/unit.h"
#include "tag_util.h"
#include "variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::units;

namespace py = pybind11;

template <class T> struct MakeVariable {
  static Variable apply(const std::vector<Dim> &labels, py::array values,
                        const std::optional<py::array> &variances,
                        const units::Unit unit) {
    // Pybind11 converts py::array to py::array_t for us, with all sorts of
    // automatic conversions such as integer to double, if required.
    py::array_t<T> valuesT(values);
    const py::buffer_info info = valuesT.request();
    Dimensions dims(labels, info.shape);
    auto var =
        variances ? makeVariableWithVariances<T>(dims) : makeVariable<T>(dims);
    copy_flattened<T>(valuesT, var.template values<T>());
    if (variances) {
      py::array_t<T> variancesT(*variances);
      const py::buffer_info info = variancesT.request();
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
using dtype = std::variant<py::dtype, scipp::core::DType>;

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
  if (type.is(py::dtype::of<int32_t>()))
    return scipp::core::dtype<int32_t>;
  if (type.is(py::dtype::of<bool>()))
    return scipp::core::dtype<bool>;
  throw std::runtime_error("Unsupported numpy dtype.");
}

scipp::core::DType scipp_dtype(const dtype &type) {
  if (std::holds_alternative<scipp::core::DType>(type))
    return std::get<scipp::core::DType>(type);
  return scipp_dtype(std::get<pybind11::dtype>(type));
}
} // namespace scippy

Variable doMakeVariable(const std::vector<Dim> &labels, py::array &values,
                        std::optional<py::array> &variances,
                        const units::Unit unit,
                        scippy::dtype type = DType::Unknown) {
  const py::buffer_info info = values.request();
  // Use custom dtype, otherwise dtype of data.
  const auto dtypeTag = scippy::scipp_dtype(type) == DType::Unknown
                            ? scippy::scipp_dtype(values.dtype())
                            : scippy::scipp_dtype(type);
  return CallDType<double, float, int64_t, int32_t, bool>::apply<MakeVariable>(
      dtypeTag, labels, values, variances, unit);
}

Variable makeVariableDefaultInit(const std::vector<Dim> &labels,
                                 const std::vector<scipp::index> &shape,
                                 const units::Unit unit, scippy::dtype type,
                                 const bool variances) {
  const auto dtypeTag = scippy::scipp_dtype(type);
  return CallDType<double, float, int64_t, int32_t, bool, Dataset,
                   Eigen::Vector3d>::apply<MakeVariableDefaultInit>(dtypeTag,
                                                                    labels,
                                                                    shape, unit,
                                                                    variances);
}

// Add size factor.
template <class T>
std::vector<scipp::index> numpy_strides(const std::vector<scipp::index> &s) {
  std::vector<scipp::index> strides(s.size());
  scipp::index elemSize = sizeof(T);
  for (size_t i = 0; i < strides.size(); ++i) {
    strides[i] = elemSize * s[i];
  }
  return strides;
}

template <class T>
auto pySlice(T &source, const std::tuple<Dim, const py::slice> &index)
    -> decltype(source(Dim::Invalid, 0)) {
  const auto & [ dim, indices ] = index;
  size_t start, stop, step, slicelength;
  const auto size = source.dims()[dim];
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (step != 1)
    throw std::runtime_error("Step must be 1");
  return source(dim, start, stop);
}

template <class T> struct MakePyBufferInfoT {
  static py::buffer_info apply(VariableProxy &view) {
    const auto &dims = view.dims();
    return py::buffer_info(
        view.template values<T>().data(), /* Pointer to buffer */
        sizeof(T),                        /* Size of one scalar */
        py::format_descriptor<
            std::conditional_t<std::is_same_v<T, bool>, bool, T>>::
            format(),              /* Python struct-style format descriptor */
        scipp::size(dims.shape()), /* Number of dimensions */
        dims.shape(),              /* Buffer dimensions */
        numpy_strides<T>(view.strides()) /* Strides (in bytes) for each index */
    );
  }
};

py::buffer_info make_py_buffer_info(VariableProxy &view) {
  return CallDType<double, float, int64_t, int32_t,
                   bool>::apply<MakePyBufferInfoT>(view.dtype(), view);
}

template <class T, class Var> auto as_py_array_t(py::object &obj, Var &view) {
  // TODO Should `Variable` also have a `strides` method?
  const auto strides = VariableProxy(view).strides();
  const auto &dims = view.dims();
  using py_T = std::conditional_t<std::is_same_v<T, bool>, bool, T>;
  return py::array_t<py_T>{dims.shape(), numpy_strides<T>(strides),
                           (py_T *)view.template values<T>().data(), obj};
}

template <class Var, class... Ts>
std::variant<py::array_t<Ts>...> as_py_array_t_variant(py::object &obj) {
  auto &view = obj.cast<Var &>();
  switch (view.dtype()) {
  case dtype<double>:
    return {as_py_array_t<double>(obj, view)};
  case dtype<float>:
    return {as_py_array_t<float>(obj, view)};
  case dtype<int64_t>:
    return {as_py_array_t<int64_t>(obj, view)};
  case dtype<int32_t>:
    return {as_py_array_t<int32_t>(obj, view)};
  case dtype<bool>:
    return {as_py_array_t<bool>(obj, view)};
  default:
    throw std::runtime_error("not implemented for this type.");
  }
}

using small_vector = boost::container::small_vector<double, 8>;
PYBIND11_MAKE_OPAQUE(small_vector);

void init_variable(py::module &m) {
  py::bind_vector<boost::container::small_vector<double, 8>>(
      m, "SmallVectorDouble8");

  py::class_<Variable> variable(m, "Variable");
  variable
      .def(py::init(&makeVariableDefaultInit),
           py::arg("labels") = std::vector<Dim>{},
           py::arg("shape") = std::vector<scipp::index>{},
           py::arg("unit") = units::Unit(units::dimensionless),
           py::arg("dtype") = dtype<double>, py::arg("variances") = false)
      .def(py::init([](const int64_t data, const units::Unit &unit) {
             auto var = makeVariable<int64_t>({}, {data});
             var.setUnit(unit);
             return var;
           }),
           py::arg("data"), py::arg("unit") = units::Unit(units::dimensionless))
      .def(py::init([](const double data, const units::Unit &unit) {
             Variable var({}, {data});
             var.setUnit(unit);
             return var;
           }),
           py::arg("data"), py::arg("unit") = units::Unit(units::dimensionless))
      // TODO Need to add overload for std::vector<std::string>, etc., see
      // Dataset.__setitem__
      .def(py::init(&doMakeVariable), py::arg("labels"), py::arg("values"),
           py::arg("variances") = std::nullopt,
           py::arg("unit") = units::Unit(units::dimensionless),
           py::arg("dtype") = DType::Unknown)
      .def(py::init<const VariableProxy &>())
      .def("__getitem__", pySlice<Variable>, py::keep_alive<0, 1>())
      .def("__setitem__",
           [](Variable &self, const std::tuple<Dim, py::slice> &index,
              const VariableProxy &other) {
             pySlice(self, index).assign(other);
           })
      .def("__setitem__",
           [](Variable &self, const std::tuple<Dim, scipp::index> &index,
              const VariableProxy &other) {
             const auto & [ dim, i ] = index;
             self(dim, i).assign(other);
           })
      .def("copy", [](const Variable &self) { return self; },
           "Make a copy of a Variable.")
      .def("__copy__", [](Variable &self) { return Variable(self); })
      .def("__deepcopy__",
           [](Variable &self, py::dict) { return Variable(self); })
      .def_property_readonly("dtype", &Variable::dtype)
      .def_property_readonly("has_variances", &Variable::hasVariances)
      .def_property_readonly(
          "numpy",
          &as_py_array_t_variant<Variable, double, float, int64_t, int32_t,
                                 bool>,
          "Returns a read-only numpy array containing the Variable's values.")
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self /= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self /= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self != py::self, py::call_guard<py::gil_scoped_release>())
      .def("__eq__", [](Variable &a, VariableProxy &b) { return a == b; },
           py::is_operator())
      .def("__ne__", [](Variable &a, VariableProxy &b) { return a != b; },
           py::is_operator())
      .def("__add__", [](Variable &a, VariableProxy &b) { return a + b; },
           py::is_operator())
      .def("__sub__", [](Variable &a, VariableProxy &b) { return a - b; },
           py::is_operator())
      .def("__mul__", [](Variable &a, VariableProxy &b) { return a * b; },
           py::is_operator())
      .def("__truediv__", [](Variable &a, VariableProxy &b) { return a / b; },
           py::is_operator())
      .def("__iadd__", [](Variable &a, VariableProxy &b) { return a += b; },
           py::is_operator())
      .def("__isub__", [](Variable &a, VariableProxy &b) { return a -= b; },
           py::is_operator())
      .def("__imul__", [](Variable &a, VariableProxy &b) { return a *= b; },
           py::is_operator())
      .def("__itruediv__", [](Variable &a, VariableProxy &b) { return a /= b; },
           py::is_operator())
      .def("__radd__", [](Variable &a, double &b) { return a + b; },
           py::is_operator())
      .def("__rsub__", [](Variable &a, double &b) { return b - a; },
           py::is_operator())
      .def("__rmul__", [](Variable &a, double &b) { return a * b; },
           py::is_operator())
      .def("__repr__",
           [](const Variable &self) { return to_string(self, "."); });

  py::class_<VariableProxy> variableProxy(m, "VariableProxy",
                                          py::buffer_protocol());
  variableProxy.def_buffer(&make_py_buffer_info);
  variableProxy
      .def("__setitem__",
           [](VariableProxy &self, const std::tuple<Dim, py::slice> &index,
              const VariableProxy &other) {
             pySlice(self, index).assign(other);
           })
      .def("__setitem__",
           [](VariableProxy &self, const std::tuple<Dim, scipp::index> &index,
              const VariableProxy &other) {
             const auto & [ dim, i ] = index;
             self(dim, i).assign(other);
           })
      .def("copy", [](const VariableProxy &self) { return Variable(self); },
           "Make a copy of a VariableProxy and return it as a Variable.")
      .def("__copy__", [](VariableProxy &self) { return Variable(self); })
      .def("__deepcopy__",
           [](VariableProxy &self, py::dict) { return Variable(self); })
      .def_property_readonly(
          "numpy",
          &as_py_array_t_variant<VariableProxy, double, float, int64_t, int32_t,
                                 bool>,
          "Returns a read-only numpy array containing the VariableProxy's "
          "values.")
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self /= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self / py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self != py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self /= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def("__eq__", [](VariableProxy &a, Variable &b) { return a == b; },
           py::is_operator())
      .def("__ne__", [](VariableProxy &a, Variable &b) { return a != b; },
           py::is_operator())
      .def("__add__", [](VariableProxy &a, Variable &b) { return a + b; },
           py::is_operator())
      .def("__sub__", [](VariableProxy &a, Variable &b) { return a - b; },
           py::is_operator())
      .def("__mul__", [](VariableProxy &a, Variable &b) { return a * b; },
           py::is_operator())
      .def("__truediv__", [](VariableProxy &a, Variable &b) { return a / b; },
           py::is_operator())
      .def("__iadd__", [](VariableProxy &a, Variable &b) { return a += b; },
           py::is_operator())
      .def("__isub__", [](VariableProxy &a, Variable &b) { return a -= b; },
           py::is_operator())
      .def("__imul__", [](VariableProxy &a, Variable &b) { return a *= b; },
           py::is_operator())
      .def("__itruediv__", [](VariableProxy &a, Variable &b) { return a /= b; },
           py::is_operator())
      .def("__radd__", [](VariableProxy &a, double &b) { return a + b; },
           py::is_operator())
      .def("__rsub__", [](VariableProxy &a, double &b) { return b - a; },
           py::is_operator())
      .def("__rmul__", [](VariableProxy &a, double &b) { return a * b; },
           py::is_operator())
      .def("reshape",
           [](const VariableProxy &self, const std::vector<Dim> &labels,
              const py::tuple &shape) {
             Dimensions dims(labels, shape.cast<std::vector<scipp::index>>());
             return self.reshape(dims);
           })
      .def("__repr__",
           [](const VariableProxy &self) { return to_string(self, "."); });

  bind_slice_methods(variable);
  bind_slice_methods(variableProxy);
  bind_data_properties(variable);
  bind_data_properties(variableProxy);

  // Implicit conversion VariableProxy -> Variable. Reduces need for excessive
  // operator overload definitions
  py::implicitly_convertible<VariableProxy, Variable>();

  m.def("split",
        py::overload_cast<const Variable &, const Dim,
                          const std::vector<scipp::index> &>(&split),
        py::call_guard<py::gil_scoped_release>(),
        "Split a Variable along a given Dimension.");
  m.def("concatenate",
        py::overload_cast<const Variable &, const Variable &, const Dim>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable containing a concatenation "
        "of two Variables along a given Dimension.");
  m.def("rebin",
        py::overload_cast<const Variable &, const Variable &, const Variable &>(
            &rebin),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable whose data is rebinned with new bin edges.");
  m.def("filter",
        py::overload_cast<const Variable &, const Variable &>(&filter),
        py::call_guard<py::gil_scoped_release>());
  m.def("sum", py::overload_cast<const Variable &, const Dim>(&sum),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable containing the sum of the data along the "
        "specified dimension.");
  m.def("mean", py::overload_cast<const Variable &, const Dim>(&mean),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable containing the mean of the data along the "
        "specified dimension.");
  m.def("norm", py::overload_cast<const Variable &>(&norm),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable containing the norm of the data.");
  // find out why py::overload_cast is not working correctly here
  m.def("sqrt", [](const Variable &self) { return sqrt(self); },
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Variable containing the square root of the data.");
}
