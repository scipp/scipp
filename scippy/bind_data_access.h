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
#include "scipp/core/dtype.h"
#include "variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::core;

template <class... Ts> struct as_VariableViewImpl {
  template <class Var>
  static std::variant<std::conditional_t<
      std::is_same_v<Var, Variable>, scipp::span<underlying_type_t<Ts>>,
      VariableView<underlying_type_t<Ts>>>...>
  values(Var &view) {
    switch (view.data().dtype()) {
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
    switch (view.data().dtype()) {
    case dtype<double>:
      return {view.template variances<double>()};
    case dtype<float>:
      return {view.template variances<float>()};
    case dtype<int64_t>:
      return {view.template variances<int64_t>()};
    case dtype<int32_t>:
      return {view.template variances<int32_t>()};
    case dtype<bool>:
      return {view.template variances<bool>()};
    case dtype<std::string>:
      return {view.template variances<std::string>()};
    case dtype<boost::container::small_vector<double, 8>>:
      return {
          view.template variances<boost::container::small_vector<double, 8>>()};
    case dtype<Dataset>:
      return {view.template variances<Dataset>()};
    case dtype<Eigen::Vector3d>:
      return {view.template variances<Eigen::Vector3d>()};
    default:
      throw std::runtime_error("not implemented for this type.");
    }
  }
  template <class Var>
  static void set_values(Var &view, const py::array &data) {
    std::visit([&data](const auto &) {}, values(view));
  }
  template <class Var>
  static void set_variances(Var &view, const py::array &data) {
    std::visit([&data](const auto &) {}, variances(view));
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
  c.def_property(
      "values", &as_VariableView::values<T>, &as_VariableView::set_values<T>,
      "Returns a read-only VariableView onto the VariableProxy's contents.");
  c.def_property(
      "variances", &as_VariableView::variances<T>,
      &as_VariableView::set_variances<T>,
      "Returns a read-only VariableView onto the VariableProxy's contents.");
  c.def_property("value", &as_VariableView::value<T>,
                 &as_VariableView::set_value<T>,
                 "The only data point for a 0-dimensional "
                 "variable. Raises an exception of the variable is "
                 "not 0-dimensional.");
  c.def_property("variances", &as_VariableView::variances<T>,
                 &as_VariableView::set_variance<T>,
                 "The only data point for a 0-dimensional "
                 "variable. Raises an exception of the variable is "
                 "not 0-dimensional.");
}

#endif // SCIPPY_BIND_DATA_ACCESS_H
