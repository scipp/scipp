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

#include "dataset.h"
#include "except.h"
#include "scipp/units/unit.h"
#include "tag_util.h"
#include "variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::units;

namespace py = pybind11;

template <typename T, typename SZ_TP>
Variable makeVariableFromBuffer(const Dimensions &dimensions,
                                const std::vector<SZ_TP> &stridesInBytes,
                                T *ptr) {
  auto ndims = scipp::size(dimensions.shape());
  if (ndims == 0) // empty dataset
    return makeVariable<underlying_type_t<T>>(dimensions);

  std::vector<SZ_TP> varStrides(ndims, 1), strides;
  for (auto &&strd : stridesInBytes)
    strides.emplace_back(strd / sizeof(T));

  bool sameStrides{*strides.rbegin() == 1};
  auto i = varStrides.size() - 1;
  while (i-- > 0) {
    varStrides[i] = varStrides[i + 1] * dimensions.size(i + 1);
    if (varStrides[i] != strides[i] && sameStrides)
      sameStrides = false;
  }

  if (sameStrides) { // memory is alligned c-style and dense
    return Variable(
        units::dimensionless, std::move(dimensions),
        Vector<underlying_type_t<T>>(ptr, ptr + dimensions.volume()));

  } else {
    // Try to find blocks to copy
    auto index = scipp::size(strides) - 1;
    while (strides[index] == varStrides[index])
      --index;
    ++index;
    auto blockSz = index < scipp::size(strides)
                       ? strides[index] * dimensions.size(index)
                       : 1;

    auto res = makeVariable<underlying_type_t<T>>(dimensions);
    std::vector<scipp::index> dsz(ndims);
    for (scipp::index i = 0; i < index; ++i)
      dsz[i] = dimensions.size(i);
    std::vector<scipp::index> coords(ndims, 0);
    auto nBlocks = dimensions.volume() / blockSz;

    for (scipp::index i = 0; i < nBlocks; ++i) {
      // calculate the array linear coordinate
      auto lin_coord = std::inner_product(coords.begin(), coords.end(),
                                          strides.begin(), scipp::index{0});
      std::memcpy(&res.template values<T>()[i * blockSz], &ptr[lin_coord],
                  blockSz * sizeof(T));
      // get the next ND coordinate
      auto k = coords.size();
      while (k-- > 0)
        ++coords[k] >= dsz[k] ? coords[k] = 0 : k = 0;
    }
    return res;
  }
}

template <typename Collection>
auto getItemBySingleIndex(Collection &self,
                          const std::tuple<Dim, scipp::index> &index) {
  scipp::index idx{std::get<scipp::index>(index)};
  auto &dim = std::get<Dim>(index);
  auto sz = self.dims()[dim];
  if (idx <= -sz || idx >= sz) // index is out of range
    throw std::runtime_error("Dimension size is " +
                             std::to_string(self.dims()[dim]) +
                             ", can't treat " + std::to_string(idx));
  if (idx < 0)
    idx = sz + idx;
  return self(std::get<Dim>(index), idx);
}

template <class T> struct mutable_span_methods {
  static void add(py::class_<scipp::span<T>> &span) {
    span.def("__setitem__", [](scipp::span<T> &self, const scipp::index i,
                               const T value) { self[i] = value; });
  }
};
template <class T> struct mutable_span_methods<const T> {
  static void add(py::class_<scipp::span<const T>> &) {}
};

template <class T> void declare_span(py::module &m, const std::string &suffix) {
  py::class_<scipp::span<T>> span(m, (std::string("span_") + suffix).c_str());
  span.def("__getitem__", &scipp::span<T>::operator[],
           py::return_value_policy::reference)
      .def("size", &scipp::span<T>::size)
      .def("__len__", &scipp::span<T>::size)
      .def("__iter__",
           [](const scipp::span<T> &self) {
             return py::make_iterator(self.begin(), self.end());
           })
      .def("__repr__",
           [](const scipp::span<T> &self) { return array_to_string(self); });
  mutable_span_methods<T>::add(span);
}

template <class T>
void declare_VariableView(py::module &m, const std::string &suffix) {
  py::class_<VariableView<T>> view(
      m, (std::string("VariableView_") + suffix).c_str());
  view.def("__repr__",
           [](const VariableView<T> &self) { return array_to_string(self); })
      .def("__getitem__", &VariableView<T>::operator[],
           py::return_value_policy::reference)
      .def("__setitem__", [](VariableView<T> &self, const scipp::index i,
                             const T value) { self[i] = value; })
      .def("__len__", &VariableView<T>::size)
      .def("__iter__", [](const VariableView<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      });
}

template <class T> struct MakeVariable {
  static Variable apply(const std::vector<Dim> &labels, py::array values,
                        const units::Unit unit) {
    // Pybind11 converts py::array to py::array_t for us, with all sorts of
    // automatic conversions such as integer to double, if required.
    py::array_t<T> valuesT(values);
    const py::buffer_info info = valuesT.request();
    Dimensions dims(labels, info.shape);
    auto *ptr = (T *)info.ptr;
    auto var = makeVariableFromBuffer<T, ssize_t>(dims, info.strides, ptr);
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
  if (variances)
    throw std::runtime_error(
        "Creating variable with variances is not implemented yet.");
  const py::buffer_info info = values.request();
  // Use custom dtype, otherwise dtype of data.
  const auto dtypeTag = scippy::scipp_dtype(type) == DType::Unknown
                            ? scippy::scipp_dtype(values.dtype())
                            : scippy::scipp_dtype(type);
  return CallDType<double, float, int64_t, int32_t, bool>::apply<MakeVariable>(
      dtypeTag, labels, values, unit);
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

template <class T> struct SetData {
  static void apply(const VariableProxy &slice, const py::array &data) {
    // Pybind11 converts py::array to py::array_t for us, with all sorts of
    // automatic conversions such as integer to double, if required.
    py::array_t<T> dataT(data);

    const auto &dims = slice.dims();
    const py::buffer_info info = dataT.request();
    const auto &shape = dims.shape();
    if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                    shape.end()))
      throw std::runtime_error(
          "Shape mismatch when setting data from numpy array.");

    auto buf = slice.values<T>();
    auto *ptr = (T *)info.ptr;
    std::copy(ptr, ptr + slice.dims().volume(), buf.begin());
  }
};

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

void setVariableProxy(VariableProxy &self,
                      const std::tuple<Dim, scipp::index> &index,
                      const py::array &data) {
  auto slice = self(std::get<Dim>(index), std::get<scipp::index>(index));
  CallDType<double, float, int64_t, int32_t, bool>::apply<SetData>(
      slice.dtype(), slice, data);
}

void setVariableProxyRange(VariableProxy &self,
                           const std::tuple<Dim, const py::slice> &index,
                           const py::array &data) {
  auto slice = pySlice(self, index);
  CallDType<double, float, int64_t, int32_t, bool>::apply<SetData>(
      slice.dtype(), slice, data);
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

template <class... Ts> struct as_VariableViewImpl {
  template <class Var>
  static std::variant<std::conditional_t<
      std::is_same_v<Var, Variable>, scipp::span<underlying_type_t<Ts>>,
      VariableView<underlying_type_t<Ts>>>...>
  values(Var &view) {
    switch (view.dtype()) {
    case dtype<double>:
      return {view.template values<double>()};
    case dtype<float>:
      return {view.template values<float>()};
    case dtype<int64_t>:
      return {view.template values<int64_t>()};
    case dtype<int32_t>:
      return {view.template values<int32_t>()};
    case dtype<bool>:
      return {view.template values<bool>()};
    case dtype<std::string>:
      return {view.template values<std::string>()};
    case dtype<boost::container::small_vector<double, 8>>:
      return {
          view.template values<boost::container::small_vector<double, 8>>()};
    case dtype<Dataset>:
      return {view.template values<Dataset>()};
    case dtype<Eigen::Vector3d>:
      return {view.template values<Eigen::Vector3d>()};
    default:
      throw std::runtime_error("not implemented for this type.");
    }
  }

  template <class Var>
  static std::variant<std::conditional_t<
      std::is_same_v<Var, Variable>, scipp::span<underlying_type_t<Ts>>,
      VariableView<underlying_type_t<Ts>>>...>
  variances(Var &view) {
    switch (view.dtype()) {
    case dtype<double>:
      return {view.template values<double>()};
    case dtype<float>:
      return {view.template values<float>()};
    case dtype<int64_t>:
      return {view.template values<int64_t>()};
    case dtype<int32_t>:
      return {view.template values<int32_t>()};
    case dtype<bool>:
      return {view.template values<bool>()};
    case dtype<std::string>:
      return {view.template values<std::string>()};
    case dtype<boost::container::small_vector<double, 8>>:
      return {
          view.template values<boost::container::small_vector<double, 8>>()};
    case dtype<Dataset>:
      return {view.template values<Dataset>()};
    case dtype<Eigen::Vector3d>:
      return {view.template values<Eigen::Vector3d>()};
    default:
      throw std::runtime_error("not implemented for this type.");
    }
  }

  // Return a scalar value from a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static py::object value(Var &view) {
    expect::equals(Dimensions(), view.dims());
    return std::visit(
        [](const auto &data) {
          return py::cast(data[0], py::return_value_policy::reference);
        },
        values(view));
  }
  // Return a scalar variance from a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static py::object variance(Var &view) {
    expect::equals(Dimensions(), view.dims());
    return std::visit(
        [](const auto &data) {
          return py::cast(data[0], py::return_value_policy::reference);
        },
        variances(view));
  }
  // Set a scalar value in a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static void set_value(Var &view, const py::object &o) {
    expect::equals(Dimensions(), view.dims());
    std::visit(
        [&o](const auto &data) {
          data[0] = o.cast<typename std::decay_t<decltype(data)>::value_type>();
        },
        values(view));
  }
  // Set a scalar variance in a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var>
  static void set_variance(Var &view, const py::object &o) {
    expect::equals(Dimensions(), view.dims());
    std::visit(
        [&o](const auto &data) {
          data[0] = o.cast<typename std::decay_t<decltype(data)>::value_type>();
        },
        variances(view));
  }
};

using as_VariableView =
    as_VariableViewImpl<double, float, int64_t, int32_t, bool, std::string,
                        boost::container::small_vector<double, 8>, Dataset,
                        Eigen::Vector3d>;

using small_vector = boost::container::small_vector<double, 8>;
PYBIND11_MAKE_OPAQUE(small_vector);

void init_variable(py::module &m) {
  py::bind_vector<boost::container::small_vector<double, 8>>(
      m, "SmallVectorDouble8");

  declare_span<double>(m, "double");
  declare_span<float>(m, "float");
  declare_span<Bool>(m, "bool");
  declare_span<const double>(m, "double_const");
  declare_span<const long>(m, "long_const");
  declare_span<long>(m, "long");
  declare_span<const std::string>(m, "string_const");
  declare_span<std::string>(m, "string");
  declare_span<const Dim>(m, "Dim_const");
  declare_span<Dataset>(m, "Dataset");
  declare_span<Eigen::Vector3d>(m, "Eigen_Vector3d");

  declare_VariableView<double>(m, "double");
  declare_VariableView<float>(m, "float");
  declare_VariableView<int64_t>(m, "int64");
  declare_VariableView<int32_t>(m, "int32");
  declare_VariableView<std::string>(m, "string");
  declare_VariableView<Bool>(m, "bool");
  declare_VariableView<boost::container::small_vector<double, 8>>(
      m, "SmallVectorDouble8");
  declare_VariableView<Dataset>(m, "Dataset");
  declare_VariableView<Eigen::Vector3d>(m, "Eigen_Vector3d");

  py::class_<Variable>(m, "Variable")
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
      .def_property("unit", &Variable::unit, &Variable::setUnit,
                    "Object of type Unit holding the unit of the Variable.")
      .def_property_readonly("dtype", &Variable::dtype)
      .def_property_readonly("has_variances", &Variable::hasVariances)
      .def_property_readonly(
          "dims", [](const Variable &self) { return self.dims(); },
          "A read-only Dimensions object containing the dimensions of the "
          "Variable.")
      .def_property_readonly(
          "numpy",
          &as_py_array_t_variant<Variable, double, float, int64_t, int32_t,
                                 bool>,
          "Returns a read-only numpy array containing the Variable's values.")
      .def_property_readonly(
          "values", &as_VariableView::values<Variable>,
          "Returns a read-only VariableView onto the Variable's contents.")
      .def_property_readonly(
          "variances", &as_VariableView::variances<Variable>,
          "Returns a read-only VariableView onto the Variable's contents.")
      .def_property("value", &as_VariableView::value<Variable>,
                    &as_VariableView::set_value<Variable>,
                    "The only value for a 0-dimensional variable. Raises an "
                    "exception if the variable is not 0-dimensional.")
      .def_property("variances", &as_VariableView::variances<Variable>,
                    &as_VariableView::set_variance<Variable>,
                    "The only data point for a 0-dimensional variable. Raises "
                    "an exception if the variable is not 0-dimensional.")
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

  py::class_<VariableProxy> view(m, "VariableProxy", py::buffer_protocol());
  view.def_buffer(&make_py_buffer_info);
  view.def_property_readonly(
          "dims", [](const VariableProxy &self) { return self.dims(); },
          py::return_value_policy::copy,
          "A read-only Dimensions object containing the dimensions of the "
          "Variable.")
      .def_property(
          "unit", &VariableProxy::unit, &VariableProxy::setUnit,
          "Object of type Unit holding the unit of the VariableProxy.")
      .def("__getitem__",
           [](VariableProxy &self, const std::tuple<Dim, scipp::index> &index) {
             return getItemBySingleIndex(self, index);
           },
           py::keep_alive<0, 1>())
      .def("__getitem__", &pySlice<VariableProxy>, py::keep_alive<0, 1>())
      .def("__getitem__",
           [](VariableProxy &self, const std::map<Dim, const scipp::index> d) {
             auto slice(self);
             for (auto item : d)
               slice = slice(item.first, item.second);
             return slice;
           },
           py::keep_alive<0, 1>())
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
      .def("__setitem__", &setVariableProxy)
      .def("__setitem__", &setVariableProxyRange)
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
      .def_property_readonly(
          "values", &as_VariableView::values<VariableProxy>,
          "Returns a read-only VariableView onto the VariableProxy's contents.")
      .def_property_readonly(
          "variances", &as_VariableView::variances<VariableProxy>,
          "Returns a read-only VariableView onto the VariableProxy's contents.")
      .def_property("value", &as_VariableView::value<VariableProxy>,
                    &as_VariableView::set_value<VariableProxy>,
                    "The only data point for a 0-dimensional "
                    "variable. Raises an exception of the variable is "
                    "not 0-dimensional.")
      .def_property("variances", &as_VariableView::variances<VariableProxy>,
                    &as_VariableView::set_variance<VariableProxy>,
                    "The only data point for a 0-dimensional "
                    "variable. Raises an exception of the variable is "
                    "not 0-dimensional.")
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
