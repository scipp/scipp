// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <variant>

#include "scipp/core/dtype.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/variable.h"

#include "numpy.h"
#include "py_object.h"
#include "pybind11.h"

namespace py = pybind11;
using namespace scipp;

template <class T> void remove_variances(T &obj) {
  if constexpr (std::is_same_v<T, DataArray> ||
                std::is_same_v<T, DataArrayView>)
    obj.data().setVariances(Variable());
  else
    obj.setVariances(Variable());
}

template <class T> void init_variances(T &obj) {
  if constexpr (std::is_same_v<T, DataArray> ||
                std::is_same_v<T, DataArrayView>)
    obj.data().setVariances(Variable(obj.data()));
  else
    obj.setVariances(Variable(obj));
}

/// Add element size as factor to strides.
template <class T>
std::vector<scipp::index> numpy_strides(const std::vector<scipp::index> &s) {
  std::vector<scipp::index> strides(s.size());
  scipp::index elemSize = sizeof(T);
  for (size_t i = 0; i < strides.size(); ++i) {
    strides[i] = elemSize * s[i];
  }
  return strides;
}

template <class T> struct MakePyBufferInfoT {
  static py::buffer_info apply(VariableView &view) {
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

inline py::buffer_info make_py_buffer_info(VariableView &view) {
  return core::CallDType<double, float, int64_t, int32_t,
                         bool>::apply<MakePyBufferInfoT>(view.dtype(), view);
}

template <class... Ts> class as_ElementArrayViewImpl;

class DataAccessHelper {
  template <class... Ts> friend class as_ElementArrayViewImpl;

  template <class Getter, class T, class Var>
  static py::object as_py_array_t_impl(py::object &obj, Var &view) {
    std::vector<scipp::index> strides;
    if constexpr (std::is_same_v<Var, DataArray> ||
                  std::is_same_v<Var, DataArrayView>) {
      strides = VariableView(view.data()).strides();
    } else {
      strides = VariableView(view).strides();
    }
    const auto &dims = view.dims();
    using py_T = std::conditional_t<std::is_same_v<T, bool>, bool, T>;
    return py::array_t<py_T>{
        dims.shape(), numpy_strides<T>(strides),
        reinterpret_cast<py_T *>(Getter::template get<T>(view).data()), obj};
  }

  struct get_values {
    template <class T, class View> static constexpr auto get(View &view) {
      return view.template values<T>();
    }

    template <class View> static bool valid(py::object &obj) {
      auto &view = obj.cast<View &>();
      if constexpr (std::is_same_v<DataArray, View> ||
                    std::is_base_of_v<DataArrayConstView, View>)
        return view.hasData() && bool(view.data());
      else
        return bool(view);
    }
  };

  struct get_variances {
    template <class T, class View> static constexpr auto get(View &view) {
      return view.template variances<T>();
    }

    template <class View> static bool valid(py::object &obj) {
      auto &view = obj.cast<View &>();
      if constexpr (std::is_same_v<DataArray, View> ||
                    std::is_base_of_v<DataArrayConstView, View>)
        return view.hasData() && view.hasVariances();
      else
        return view.hasVariances();
    }
  };
};

template <class... Ts> class as_ElementArrayViewImpl {
  using get_values = DataAccessHelper::get_values;
  using get_variances = DataAccessHelper::get_variances;

  template <class View>
  using outVariant_t = std::variant<
      std::conditional_t<std::is_same_v<View, Variable>, scipp::span<Ts>,
                         ElementArrayView<Ts>>...>;

  template <class Getter, class View>
  static outVariant_t<View> get(View &view) {
    DType type = view.data().dtype();
    if constexpr (std::is_same_v<DataArray, View> ||
                  std::is_base_of_v<DataArrayConstView, View>) {
      const auto &v = view.data();
      type = v.data().dtype();
    }
    if (type == dtype<double>)
      return {Getter::template get<double>(view)};
    if (type == dtype<float>)
      return {Getter::template get<float>(view)};
    if (type == dtype<int64_t>)
      return {Getter::template get<int64_t>(view)};
    if (type == dtype<int32_t>)
      return {Getter::template get<int32_t>(view)};
    if (type == dtype<bool>)
      return {Getter::template get<bool>(view)};
    if (type == dtype<std::string>)
      return {Getter::template get<std::string>(view)};
    if (type == dtype<event_list<double>>)
      return {Getter::template get<event_list<double>>(view)};
    if (type == dtype<event_list<float>>)
      return {Getter::template get<event_list<float>>(view)};
    if (type == dtype<event_list<int64_t>>)
      return {Getter::template get<event_list<int64_t>>(view)};
    if (type == dtype<DataArray>)
      return {Getter::template get<DataArray>(view)};
    if (type == dtype<Dataset>)
      return {Getter::template get<Dataset>(view)};
    if (type == dtype<Eigen::Vector3d>)
      return {Getter::template get<Eigen::Vector3d>(view)};
    if (type == dtype<Eigen::Matrix3d>)
      return {Getter::template get<Eigen::Matrix3d>(view)};
    if (type == dtype<scipp::python::PyObject>)
      return {Getter::template get<scipp::python::PyObject>(view)};
    throw std::runtime_error("not implemented for this type.");
  }

  template <class View>
  static void set(const Dimensions &dims, const View &view,
                  const py::object &obj) {
    std::visit(
        [&dims, &obj](const auto &view_) {
          using T =
              typename std::remove_reference_t<decltype(view_)>::value_type;
          if constexpr (std::is_trivial_v<T>) {
            auto &data = obj.cast<const py::array_t<T>>();
            const auto &shape = dims.shape();
            if (!std::equal(shape.begin(), shape.end(), data.shape(),
                            data.shape() + data.ndim()))
              throw except::DimensionError("The shape of the provided data "
                                           "does not match the existing "
                                           "object.");
            copy_flattened<T>(data, view_);
          } else if constexpr (core::is_events_v<T>) {
            auto &data = obj.cast<const py::array_t<typename T::value_type>>();
            // Event data can be set from an array only for a single item.
            if (dims.shape().size() != 0)
              throw except::DimensionError(
                  "Event data cannot be set from a single "
                  "array, unless the events dimension is the "
                  "only dimension.");
            if (data.ndim() != 1)
              throw except::DimensionError("Expected 1-D data.");
            auto r = data.unchecked();
            view_[0].clear();
            for (ssize_t i = 0; i < r.shape(0); ++i)
              view_[0].emplace_back(r(i));
          } else {
            const auto &data = obj.cast<const std::vector<T>>();
            // TODO Related to #290, we should properly support
            // multi-dimensional input, and ignore bad shapes.
            core::expect::sizeMatches(view_, data);
            std::copy(data.begin(), data.end(), view_.begin());
          }
        },
        view);
  }

  template <class Getter, class View>
  static py::object get_py_array_t(py::object &obj) {
    auto &view = obj.cast<View &>();
    DType type = view.data().dtype();
    if constexpr (std::is_same_v<DataArray, View> ||
                  std::is_base_of_v<DataArrayConstView, View>) {
      const auto &v = view.data();
      type = v.data().dtype();
    }
    if (type == dtype<double>)
      return DataAccessHelper::as_py_array_t_impl<Getter, double>(obj, view);
    if (type == dtype<float>)
      return DataAccessHelper::as_py_array_t_impl<Getter, float>(obj, view);
    if (type == dtype<int64_t>)
      return DataAccessHelper::as_py_array_t_impl<Getter, int64_t>(obj, view);
    if (type == dtype<int32_t>)
      return DataAccessHelper::as_py_array_t_impl<Getter, int32_t>(obj, view);
    if (type == dtype<bool>)
      return DataAccessHelper::as_py_array_t_impl<Getter, bool>(obj, view);
    return std::visit(
        [&view, &obj](const auto &data) {
          const auto &dims = view.dims();
          // We return an individual item in two cases:
          // 1. For 0-D data (consistent with numpy behavior, e.g., when slicing
          // a 1-D array).
          // 2. For 1-D event data, where the individual item is then a
          // vector-like object.
          if (dims.shape().size() == 0) {
            if constexpr (std::is_same_v<std::decay_t<decltype(data[0])>,
                                         scipp::python::PyObject>) {
              // Returning PyObject. This increments the reference counter of
              // the element, so it is ok if the parent `obj` (the variable)
              // goes out of scope.
              return data[0].to_pybind();
            } else {
              // Returning reference to element in variable. Return-policy
              // reference_internal keeps alive `obj`. Note that an attempt to
              // pass `keep_alive` as a call policy to `def_property` failed,
              // resulting in exception from pybind11, so we have handle it by
              // hand here.
              return py::cast(data[0],
                              py::return_value_policy::reference_internal, obj);
            }
          } else {
            // Returning view (span or ElementArrayView) by value. This
            // references data in variable, so it must be kept alive. There is
            // no policy that supports this, so we use `keep_alive_impl`
            // manually.
            auto ret = py::cast(data, py::return_value_policy::move);
            pybind11::detail::keep_alive_impl(ret, obj);
            return ret;
          }
        },
        get<Getter>(view));
  }

public:
  template <class Var> static py::object values(py::object &object) {
    if (!get_values::valid<Var>(object))
      return py::none();
    return get_py_array_t<get_values, Var>(object);
  }

  template <class Var> static py::object variances(py::object &object) {
    if (!get_variances::valid<Var>(object))
      return py::none();
    return get_py_array_t<get_variances, Var>(object);
  }

  template <class Var>
  static void set_values(Var &view, const py::object &obj) {
    set(view.dims(), get<get_values>(view), obj);
  }

  template <class Var>
  static void set_variances(Var &view, const py::object &obj) {
    if (obj.is_none())
      return remove_variances(view);
    if (!view.hasVariances())
      init_variances(view);
    set(view.dims(), get<get_variances>(view), obj);
  }

  // Return a scalar value from a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static py::object value(py::object &obj) {
    if (!get_values::valid<Var>(obj))
      return py::none();
    auto &view = obj.cast<Var &>();
    core::expect::equals(Dimensions(), view.dims());
    return std::visit(
        [&obj](const auto &data) {
          if constexpr (std::is_same_v<std::decay_t<decltype(data[0])>,
                                       scipp::python::PyObject>)
            return data[0].to_pybind();
          else
            // Passing `obj` as parent so py::keep_alive works.
            return py::cast(data[0],
                            py::return_value_policy::reference_internal, obj);
        },
        get<get_values>(view));
  }
  // Return a scalar variance from a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static py::object variance(py::object &obj) {
    if (!get_variances::valid<Var>(obj))
      return py::none();
    auto &view = obj.cast<Var &>();
    core::expect::equals(Dimensions(), view.dims());
    return std::visit(
        [&obj](const auto &data) {
          if constexpr (std::is_same_v<std::decay_t<decltype(data[0])>,
                                       scipp::python::PyObject>)
            return data[0].to_pybind();
          else
            // Passing `obj` as parent so py::keep_alive works.
            return py::cast(data[0],
                            py::return_value_policy::reference_internal, obj);
        },
        get<get_variances>(view));
  }
  // Set a scalar value in a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static void set_value(Var &view, const py::object &o) {
    core::expect::equals(Dimensions(), view.dims());
    std::visit(
        [&o](const auto &data) {
          using T = typename std::decay_t<decltype(data)>::value_type;
          if constexpr (std::is_same_v<T, scipp::python::PyObject>)
            data[0] = o;
          else
            data[0] = o.cast<T>();
        },
        get<get_values>(view));
  }
  // Set a scalar variance in a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var>
  static void set_variance(Var &view, const py::object &o) {
    core::expect::equals(Dimensions(), view.dims());
    if (o.is_none())
      return remove_variances(view);
    if (!view.hasVariances())
      init_variances(view);

    std::visit(
        [&o](const auto &data) {
          using T = typename std::decay_t<decltype(data)>::value_type;
          if constexpr (std::is_same_v<T, scipp::python::PyObject>)
            data[0] = o;
          else
            data[0] = o.cast<T>();
        },
        get<get_variances>(view));
  }
};

using as_ElementArrayView = as_ElementArrayViewImpl<
    double, float, int64_t, int32_t, bool, std::string, event_list<double>,
    event_list<float>, event_list<int64_t>, DataArray, Dataset, Eigen::Vector3d,
    Eigen::Matrix3d, scipp::python::PyObject>;

template <class T, class... Ignored>
void bind_data_properties(pybind11::class_<T, Ignored...> &c) {
  c.def_property_readonly(
      "dtype", [](const T &self) { return self.dtype(); },
      "Data type contained in the variable.");
  c.def_property_readonly(
      "dims",
      [](const T &self) {
        const auto &dims = self.dims();
        return std::vector<Dim>(dims.labels().begin(), dims.labels().end());
      },
      "Dimension labels of the data (read-only).",
      py::return_value_policy::move);
  c.def_property_readonly(
      "shape",
      [](const T &self) {
        const auto &dims = self.dims();
        return std::vector<int64_t>(dims.shape().begin(), dims.shape().end());
      },
      "Shape of the data (read-only).", py::return_value_policy::move);

  c.def_property(
      "unit", [](const T &self) { return self.unit(); }, &T::setUnit,
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
}
