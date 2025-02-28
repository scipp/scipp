// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <variant>

#include <pybind11/typing.h>

#include "scipp/core/dtype.h"
#include "scipp/core/eigen.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_concept.h"

#include "dtype.h"
#include "numpy.h"
#include "py_object.h"
#include "pybind11.h"
#include "unit.h"

namespace py = pybind11;
using namespace scipp;

template <class T> void remove_variances(T &obj) {
  if constexpr (std::is_same_v<T, DataArray>)
    obj.data().setVariances(Variable());
  else
    obj.setVariances(Variable());
}

template <class T> void init_variances(T &obj) {
  if constexpr (std::is_same_v<T, DataArray>)
    obj.data().setVariances(Variable(obj.data()));
  else
    obj.setVariances(Variable(obj));
}

/// Add element size as factor to strides.
template <class T>
std::vector<scipp::index>
numpy_strides(const scipp::span<const scipp::index> &s) {
  std::vector<scipp::index> strides(s.size());
  for (size_t i = 0; i < strides.size(); ++i) {
    strides[i] = sizeof(T) * s[i];
  }
  return strides;
}

template <typename T> decltype(auto) get_data_variable(T &&x) {
  if constexpr (std::is_same_v<std::decay_t<T>, scipp::Variable>) {
    return std::forward<T>(x);
  } else {
    return std::forward<T>(x).data();
  }
}

/// Return a pybind11 handle to the VariableConcept of x.
/// Refers to the data variable if T is a DataArray.
template <typename T> auto get_data_variable_concept_handle(T &&x) {
  return py::cast(get_data_variable(std::forward<T>(x)).data_handle());
}

template <class... Ts> class as_ElementArrayViewImpl;

class DataAccessHelper {
  template <class... Ts> friend class as_ElementArrayViewImpl;

  template <class Getter, class T, class View>
  static py::object as_py_array_t_impl(View &&view) {
    const auto get_dtype = [&view]() {
      if constexpr (std::is_same_v<T, scipp::core::time_point>) {
        // Need a custom implementation because py::dtype::of only works with
        // types supported by the buffer protocol.
        return py::dtype("datetime64[" + to_numpy_time_string(view.unit()) +
                         ']');
      } else {
        static_cast<void>(view);
        return py::dtype::of<T>();
      }
    };
    auto &&var = get_data_variable(view);
    const auto &dims = view.dims();
    if (var.is_readonly()) {
      auto array =
          py::array{get_dtype(), dims.shape(), numpy_strides<T>(var.strides()),
                    Getter::template get<T>(std::as_const(view)).data(),
                    get_data_variable_concept_handle(view)};
      py::detail::array_proxy(array.ptr())->flags &=
          ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
      // no automatic move because of type mismatch
      return py::object{std::move(array)};
    } else {
      return py::array{get_dtype(), dims.shape(),
                       numpy_strides<T>(var.strides()),
                       Getter::template get<T>(view).data(),
                       get_data_variable_concept_handle(view)};
    }
  }

  struct get_values {
    template <class T, class View> static constexpr auto get(View &&view) {
      return view.template values<T>();
    }
  };

  struct get_variances {
    template <class T, class View> static constexpr auto get(View &&view) {
      return view.template variances<T>();
    }
  };
};

inline void expect_scalar(const Dimensions &dims, const std::string_view name) {
  if (dims != Dimensions{}) {
    std::ostringstream oss;
    oss << "The '" << name << "' property cannot be used with non-scalar "
        << "Variables. Got dimensions " << to_string(dims) << ". Did you mean '"
        << name << "s'?";
    throw except::DimensionError(oss.str());
  }
}

template <class... Ts> class as_ElementArrayViewImpl {
  using get_values = DataAccessHelper::get_values;
  using get_variances = DataAccessHelper::get_variances;

  template <class View>
  using outVariant_t = std::variant<ElementArrayView<Ts>...>;

  template <class Getter, class View>
  static outVariant_t<View> get(View &view) {
    const DType type = view.dtype();
    if (type == dtype<double>)
      return {Getter::template get<double>(view)};
    if (type == dtype<float>)
      return {Getter::template get<float>(view)};
    if constexpr (std::is_same_v<Getter, get_values>) {
      if (type == dtype<int64_t>)
        return {Getter::template get<int64_t>(view)};
      if (type == dtype<int32_t>)
        return {Getter::template get<int32_t>(view)};
      if (type == dtype<bool>)
        return {Getter::template get<bool>(view)};
      if (type == dtype<std::string>)
        return {Getter::template get<std::string>(view)};
      if (type == dtype<scipp::core::time_point>)
        return {Getter::template get<scipp::core::time_point>(view)};
      if (type == dtype<Variable>)
        return {Getter::template get<Variable>(view)};
      if (type == dtype<DataArray>)
        return {Getter::template get<DataArray>(view)};
      if (type == dtype<Dataset>)
        return {Getter::template get<Dataset>(view)};
      if (type == dtype<Eigen::Vector3d>)
        return {Getter::template get<Eigen::Vector3d>(view)};
      if (type == dtype<Eigen::Matrix3d>)
        return {Getter::template get<Eigen::Matrix3d>(view)};
      if (type == dtype<Eigen::Affine3d>)
        return {Getter::template get<Eigen::Affine3d>(view)};
      if (type == dtype<scipp::core::Quaternion>)
        return {Getter::template get<scipp::core::Quaternion>(view)};
      if (type == dtype<scipp::core::Translation>)
        return {Getter::template get<scipp::core::Translation>(view)};
      if (type == dtype<scipp::python::PyObject>)
        return {Getter::template get<scipp::python::PyObject>(view)};
      if (type == dtype<bucket<Variable>>)
        return {Getter::template get<bucket<Variable>>(view)};
      if (type == dtype<bucket<DataArray>>)
        return {Getter::template get<bucket<DataArray>>(view)};
      if (type == dtype<bucket<Dataset>>)
        return {Getter::template get<bucket<Dataset>>(view)};
    }
    throw std::runtime_error("Value-access not implemented for this type.");
  }

  template <class View>
  static void set(const Dimensions &dims, const units::Unit unit,
                  const View &view, const py::object &obj) {
    std::visit(
        [&dims, &unit, &obj](const auto &view_) {
          using T =
              typename std::remove_reference_t<decltype(view_)>::value_type;
          copy_array_into_view(cast_to_array_like<T>(obj, unit), view_, dims);
        },
        view);
  }

  template <typename View, typename T>
  static auto
  get_matrix_elements(const View &view,
                      const std::initializer_list<scipp::index> shape) {
    auto elems = get_data_variable(view).template elements<T>();
    elems = fold(
        elems, Dim::InternalStructureComponent,
        Dimensions({Dim::InternalStructureRow, Dim::InternalStructureColumn},
                   shape));
    std::vector labels(elems.dims().labels().begin(),
                       elems.dims().labels().end());
    std::iter_swap(labels.end() - 2, labels.end() - 1);
    return transpose(elems, labels);
  }

  template <class View> static auto structure_elements(View &&view) {
    if (view.dtype() == dtype<Eigen::Vector3d>) {
      return get_data_variable(view).template elements<Eigen::Vector3d>();
    } else if (view.dtype() == dtype<Eigen::Matrix3d>) {
      return get_matrix_elements<View, Eigen::Matrix3d>(view, {3, 3});
    } else if (view.dtype() == dtype<scipp::core::Quaternion>) {
      return get_data_variable(view)
          .template elements<scipp::core::Quaternion>();
    } else if (view.dtype() == dtype<scipp::core::Translation>) {
      return get_data_variable(view)
          .template elements<scipp::core::Translation>();
    } else if (view.dtype() == dtype<Eigen::Affine3d>) {
      return get_matrix_elements<View, Eigen::Affine3d>(view, {4, 4});
    } else {
      throw std::runtime_error("Unsupported structured dtype");
    }
  }

public:
  template <class Getter, class View>
  static py::object get_py_array_t(py::object &obj) {
    auto &view = obj.cast<View &>();
    if (!std::is_const_v<View> && get_data_variable(view).is_readonly())
      return as_ElementArrayViewImpl<const Ts...>::template get_py_array_t<
          Getter, const View>(obj);
    const DType type = view.dtype();
    if (type == dtype<double>)
      return DataAccessHelper::as_py_array_t_impl<Getter, double>(view);
    if (type == dtype<float>)
      return DataAccessHelper::as_py_array_t_impl<Getter, float>(view);
    if (type == dtype<int64_t>)
      return DataAccessHelper::as_py_array_t_impl<Getter, int64_t>(view);
    if (type == dtype<int32_t>)
      return DataAccessHelper::as_py_array_t_impl<Getter, int32_t>(view);
    if (type == dtype<bool>)
      return DataAccessHelper::as_py_array_t_impl<Getter, bool>(view);
    if (type == dtype<scipp::core::time_point>)
      return DataAccessHelper::as_py_array_t_impl<Getter,
                                                  scipp::core::time_point>(
          view);
    if (is_structured(type))
      return DataAccessHelper::as_py_array_t_impl<Getter, double>(
          structure_elements(view));
    return std::visit(
        [&view](const auto &data) {
          const auto &dims = view.dims();
          // We return an individual item in two cases:
          // 1. For 0-D data (consistent with numpy behavior, e.g., when slicing
          // a 1-D array).
          // 2. For 1-D event data, where the individual item is then a
          // vector-like object.
          if (dims.ndim() == 0) {
            return make_scalar(data[0], get_data_variable_concept_handle(view),
                               view);
          } else {
            // Returning view (span or ElementArrayView) by value. This
            // references data in variable, so it must be kept alive. There is
            // no policy that supports this, so we use `keep_alive_impl`
            // manually.
            auto ret = py::cast(data, py::return_value_policy::move);
            pybind11::detail::keep_alive_impl(
                ret, get_data_variable_concept_handle(view));
            return ret;
          }
        },
        get<Getter>(view));
  }

  template <class Var> static py::object values(py::object &object) {
    return get_py_array_t<get_values, Var>(object);
  }

  template <class Var> static py::object variances(py::object &object) {
    if (!object.cast<Var &>().has_variances())
      return py::none();
    return get_py_array_t<get_variances, Var>(object);
  }

  template <class Var>
  static void set_values(Var &view, const py::object &obj) {
    if (is_structured(view.dtype())) {
      auto elems = structure_elements(view);
      set_values(elems, obj);
    } else {
      set(view.dims(), view.unit(), get<get_values>(view), obj);
    }
  }

  template <class Var>
  static void set_variances(Var &view, const py::object &obj) {
    if (obj.is_none())
      return remove_variances(view);
    if (!view.has_variances())
      init_variances(view);
    set(view.dims(), view.unit(), get<get_variances>(view), obj);
  }

private:
  static auto numpy_attr(const char *const name) {
    return py::module_::import("numpy").attr(name);
  }

  template <class Scalar, class View>
  static auto make_scalar(Scalar &&scalar, py::object parent,
                          const View &view) {
    if constexpr (std::is_same_v<std::decay_t<Scalar>,
                                 scipp::python::PyObject>) {
      // Returning PyObject. This increments the reference counter of
      // the element, so it is ok if the parent `parent` (the variable)
      // goes out of scope.
      return scalar.to_pybind();
    } else if constexpr (std::is_same_v<std::decay_t<Scalar>,
                                        core::time_point>) {
      const auto np_datetime64 = numpy_attr("datetime64");
      return np_datetime64(scalar.time_since_epoch(),
                           to_numpy_time_string(view.unit()));
    } else if constexpr (std::is_same_v<std::decay_t<Scalar>, int32_t>) {
      return numpy_attr("int32")(scalar);
    } else if constexpr (std::is_same_v<std::decay_t<Scalar>, int64_t>) {
      return numpy_attr("int64")(scalar);
    } else if constexpr (std::is_same_v<std::decay_t<Scalar>, float>) {
      return numpy_attr("float32")(scalar);
    } else if constexpr (std::is_same_v<std::decay_t<Scalar>, double>) {
      return numpy_attr("float64")(scalar);
    } else if constexpr (!std::is_reference_v<Scalar>) {
      // Views such as slices of data arrays for binned data are
      // returned by value and require separate handling to avoid the
      // py::return_value_policy::reference_internal in the default case
      // below.
      return py::cast(scalar, py::return_value_policy::move);
    } else {
      // Returning reference to element in variable. Return-policy
      // reference_internal keeps alive `parent`. Note that an attempt to
      // pass `keep_alive` as a call policy to `def_property` failed,
      // resulting in exception from pybind11, so we have to handle it by
      // hand here.
      return py::cast(scalar, py::return_value_policy::reference_internal,
                      std::move(parent));
    }
  }

  // Helper function object to get a scalar value or variance.
  template <class View> struct GetScalarVisitor {
    py::object &self; // The object we're getting the value / variance from.
    std::remove_reference_t<View> &view; // self as a view.

    template <class Data> auto operator()(const Data &&data) const {
      return make_scalar(data[0], self, view);
    }
  };

  // Helper function object to set a scalar value or variance.
  template <class View> struct SetScalarVisitor {
    const py::object &rhs;               // The object we are assigning.
    std::remove_reference_t<View> &view; // View of self.

    template <class Data> auto operator()(Data &&data) const {
      using T = typename std::decay_t<decltype(data)>::value_type;
      if constexpr (std::is_same_v<T, scipp::python::PyObject>)
        data[0] = rhs;
      else if constexpr (std::is_same_v<T, scipp::core::time_point>) {
        // TODO support int
        if (view.unit() != parse_datetime_dtype(rhs)) {
          // TODO implement
          throw std::invalid_argument(
              "Conversion of time units is not implemented.");
        }
        data[0] = make_time_point(rhs.template cast<py::buffer>());
      } else
        data[0] = rhs.cast<T>();
    }
  };

public:
  // Return a scalar value from a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static py::object value(py::object &obj) {
    auto &view = obj.cast<Var &>();
    if (!std::is_const_v<Var> && get_data_variable(view).is_readonly())
      return as_ElementArrayViewImpl<const Ts...>::template value<const Var>(
          obj);
    expect_scalar(view.dims(), "value");
    if (view.dtype() == dtype<scipp::core::Quaternion> ||
        view.dtype() == dtype<scipp::core::Translation> ||
        view.dtype() == dtype<Eigen::Affine3d>)
      return get_py_array_t<get_values, Var>(obj);
    return std::visit(GetScalarVisitor<decltype(view)>{obj, view},
                      get<get_values>(view));
  }
  // Return a scalar variance from a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static py::object variance(py::object &obj) {
    auto &view = obj.cast<Var &>();
    if (!std::is_const_v<Var> && get_data_variable(view).is_readonly())
      return as_ElementArrayViewImpl<const Ts...>::template variance<const Var>(
          obj);
    expect_scalar(view.dims(), "variance");
    if (!view.has_variances())
      return py::none();
    return std::visit(GetScalarVisitor<decltype(view)>{obj, view},
                      get<get_variances>(view));
  }
  // Set a scalar value in a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static void set_value(Var &view, const py::object &obj) {
    expect_scalar(view.dims(), "value");
    if (is_structured(view.dtype())) {
      auto elems = structure_elements(view);
      set_values(elems, obj);
    } else {
      std::visit(SetScalarVisitor<decltype(view)>{obj, view},
                 get<get_values>(view));
    }
  }
  // Set a scalar variance in a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var>
  static void set_variance(Var &view, const py::object &obj) {
    expect_scalar(view.dims(), "variance");
    if (obj.is_none())
      return remove_variances(view);
    if (!view.has_variances())
      init_variances(view);

    std::visit(SetScalarVisitor<decltype(view)>{obj, view},
               get<get_variances>(view));
  }
};

using as_ElementArrayView = as_ElementArrayViewImpl<
    double, float, int64_t, int32_t, bool, std::string, scipp::core::time_point,
    Variable, DataArray, Dataset, bucket<Variable>, bucket<DataArray>,
    bucket<Dataset>, Eigen::Vector3d, Eigen::Matrix3d, scipp::python::PyObject,
    Eigen::Affine3d, scipp::core::Quaternion, scipp::core::Translation>;

template <class T, class... Ignored>
void bind_common_data_properties(pybind11::class_<T, Ignored...> &c) {
  c.def_property_readonly(
      "dims",
      [](const T &self) {
        const auto &labels = self.dims().labels();
        const auto ndim = static_cast<size_t>(self.ndim());
        py::typing::Tuple<py::str, py::ellipsis> dims(ndim);
        for (size_t i = 0; i < ndim; ++i) {
          dims[i] = labels[i].name();
        }
        return dims;
      },
      "Dimension labels of the data (read-only).",
      py::return_value_policy::move);
  c.def_property_readonly(
      "dim", [](const T &self) { return self.dim().name(); },
      "The only dimension label for 1-dimensional data, raising an exception "
      "if the data is not 1-dimensional.");
  c.def_property_readonly(
      "ndim", [](const T &self) { return self.ndim(); },
      "Number of dimensions of the data (read-only).",
      py::return_value_policy::move);
  c.def_property_readonly(
      "shape",
      [](const T &self) {
        const auto &sizes = self.dims().sizes();
        const auto ndim = static_cast<size_t>(self.ndim());
        py::typing::Tuple<int, py::ellipsis> shape(ndim);
        for (size_t i = 0; i < ndim; ++i) {
          shape[i] = sizes[i];
        }
        return shape;
      },
      "Shape of the data (read-only).", py::return_value_policy::move);
  c.def_property_readonly(
      "sizes",
      [](const T &self) {
        const auto &dims = self.dims();
        // Use py::dict directly instead of std::map in order to guarantee
        // that items are stored in the order of insertion.
        py::typing::Dict<py::str, int> sizes;
        for (const auto label : dims.labels()) {
          sizes[label.name().c_str()] = dims[label];
        }
        return sizes;
      },
      "dict mapping dimension labels to dimension sizes (read-only).",
      py::return_value_policy::move);
}

namespace {
template <class T>
Variable get_data_variable(const T &self, const std::string &property_name) {
  Variable var;
  if constexpr (std::is_same_v<T, DataArray>)
    var = self.data();
  else
    var = self;
  const auto dt = var.dtype();
  if (dt == dtype<bucket<Variable>>) {
    var = var.template bin_buffer<Variable>();
  } else if (dt == dtype<bucket<DataArray>>) {
    var = var.template bin_buffer<DataArray>().data();
  } else if (dt == dtype<bucket<Dataset>>) {
    throw std::runtime_error("Binned data with content of type Dataset "
                             "does not have a well-defined " +
                             property_name + ".");
  }
  return var;
}
} // namespace

template <class T, class... Ignored>
void bind_data_properties(pybind11::class_<T, Ignored...> &c) {
  bind_common_data_properties(c);
  c.def_property_readonly(
      "dtype",
      [](const T &self) { return get_data_variable(self, "dtype").dtype(); },
      "Data type contained in the variable.");
  c.def_property(
      "unit",
      [](const T &self) {
        const auto &var = get_data_variable(self, "unit");
        return var.unit() == units::none ? std::optional<units::Unit>()
                                         : var.unit();
      },
      [](const T &self, const ProtoUnit &unit) {
        auto var = get_data_variable(self, "unit");
        var.setUnit(unit_or_default(unit, var.dtype()));
      },
      "Physical unit of the data.");
  c.def_property("values", &as_ElementArrayView::values<T>,
                 &as_ElementArrayView::set_values<T>,
                 "Array of values of the data.");
  c.def_property("variances", &as_ElementArrayView::variances<T>,
                 &as_ElementArrayView::set_variances<T>,
                 "Array of variances of the data.");
  c.def_property(
      "value", &as_ElementArrayView::value<T>,
      &as_ElementArrayView::set_value<T>,
      "The only value for 0-dimensional data, raising an exception if the data "
      "is not 0-dimensional.");
  c.def_property(
      "variance", &as_ElementArrayView::variance<T>,
      &as_ElementArrayView::set_variance<T>,
      "The only variance for 0-dimensional data, raising an exception if the "
      "data is not 0-dimensional.");
  if constexpr (std::is_same_v<T, DataArray> || std::is_same_v<T, Variable>) {
    c.def_property_readonly(
        "size", [](const T &self) { return self.dims().volume(); },
        "Number of elements in the data (read-only).",
        py::return_value_policy::move);
  }
}
