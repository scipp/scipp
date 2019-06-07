// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPPY_BIND_DATA_ACCESS_H
#define SCIPPY_BIND_DATA_ACCESS_H

#include <variant>

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>

#include "except.h"
#include "numpy.h"
#include "scipp/core/dtype.h"
#include "variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::core;

struct get_values {
  template <class T, class Proxy> static constexpr auto get(Proxy &proxy) {
    return proxy.template values<T>();
  }
};
struct get_variances {
  template <class T, class Proxy> static constexpr auto get(Proxy &proxy) {
    return proxy.template variances<T>();
  }
};

template <class... Ts> struct as_VariableViewImpl {
  template <class Getter, class Var>
  static std::variant<std::conditional_t<
      std::is_same_v<Var, Variable>, scipp::span<underlying_type_t<Ts>>,
      VariableView<underlying_type_t<Ts>>>...>
  get(Var &view) {
    switch (view.data().dtype()) {
    case dtype<double>:
      return {Getter::template get<double>(view)};
    case dtype<float>:
      return {Getter::template get<float>(view)};
    case dtype<int64_t>:
      return {Getter::template get<int64_t>(view)};
    case dtype<int32_t>:
      return {Getter::template get<int32_t>(view)};
    case dtype<bool>:
      return {Getter::template get<bool>(view)};
    case dtype<std::string>:
      return {Getter::template get<std::string>(view)};
    case dtype<boost::container::small_vector<double, 8>>:
      return {Getter::template get<boost::container::small_vector<double, 8>>(
          view)};
    case dtype<Dataset>:
      return {Getter::template get<Dataset>(view)};
    case dtype<Eigen::Vector3d>:
      return {Getter::template get<Eigen::Vector3d>(view)};
    default:
      throw std::runtime_error("not implemented for this type.");
    }
  }
  template <class Var>
  static auto values(Var &view) -> decltype(get<get_values>(view)) {
    return get<get_values>(view);
  }
  template <class Var>
  static auto variances(Var &view) -> decltype(get<get_variances>(view)) {
    return get<get_variances>(view);
  }

  template <class Proxy>
  static void set(const Proxy &proxy, const py::array &data) {
    std::visit(
        [&data](const auto &proxy) {
          using T =
              typename std::remove_reference_t<decltype(proxy)>::value_type;
          if constexpr (std::is_trivial_v<T>) {
            copy_flattened<T>(data, proxy);
          } else {
            throw std::runtime_error("Only POD types can be set from numpy.");
          }
        },
        proxy);
  }
  template <class Var>
  static void set_values(Var &view, const py::array &data) {
    set(values(view), data);
  }
  template <class Var>
  static void set_variances(Var &view, const py::array &data) {
    set(variances(view), data);
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

template <class T, class... Ignored>
void bind_data_properties(pybind11::class_<T, Ignored...> &c) {
  c.def_property_readonly("dims", [](const T &self) { return self.dims(); },
                          py::return_value_policy::copy,
                          "The dimensions of the data (read-only).");
  c.def_property("unit", &T::unit, &T::setUnit,
                 "The physical unit of the data (writable).");

  c.def_property("values", &as_VariableView::values<T>,
                 &as_VariableView::set_values<T>,
                 "The array of values of the data (writable).");
  c.def_property("variances", &as_VariableView::variances<T>,
                 &as_VariableView::set_variances<T>,
                 "The array of variances of the data (writable).");
  c.def_property("value", &as_VariableView::value<T>,
                 &as_VariableView::set_value<T>,
                 "The only value for 0-dimensional data. Raises an exception "
                 "of the data is not 0-dimensional.");
  c.def_property("variance", &as_VariableView::variance<T>,
                 &as_VariableView::set_variance<T>,
                 "The only variance for 0-dimensional data. Raises an "
                 "exception of the data is not 0-dimensional.");
  c.def_property_readonly("has_variances", &T::hasVariances);
}

#endif // SCIPPY_BIND_DATA_ACCESS_H
