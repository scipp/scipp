// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/array_to_string.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/core/has_eval.h"
#include "scipp/units/unit.h"
#include "scipp/variable/data_model.h"
#include "scipp/variable/except.h"
#include "scipp/variable/structured_model.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

namespace {

template <class T> struct model { using type = DataModel<T>; };
template <> struct model<Eigen::Vector3d> {
  using type = StructuredModel<Eigen::Vector3d, double, 3>;
};
template <> struct model<Eigen::Matrix3d> {
  using type = StructuredModel<Eigen::Matrix3d, double, 3, 3>;
};
template <class T> using model_t = typename model<T>::type;

template <class T> const auto &cast(const Variable &var) {
  return requireT<const model_t<T>>(var.data());
}

template <class T> auto &cast(Variable &var) {
  return requireT<model_t<T>>(var.data());
}

template <class T> static Dimensions inner_dims{};
template <> Dimensions inner_dims<Eigen::Vector3d>{Dim::Internal0, 3};
template <>
Dimensions inner_dims<Eigen::Matrix3d>{{Dim::Internal0, 3},
                                       {Dim::Internal1, 3}};

template <class T>
auto make_model(const units::Unit unit, const Dimensions &dimensions,
                element_array<T> values,
                std::optional<element_array<T>> variances) {
  if constexpr (std::is_same_v<model_t<T>, DataModel<T>>) {
    return std::make_unique<model_t<T>>(
        dimensions.volume(), unit, std::move(values), std::move(variances));
  } else {
    // There is an extra copy caused here, but in practice this constructor
    // should not be used much outside unit tests.
    using Elem = typename model_t<T>::element_type;
    element_array<Elem> elems;
    if (values) {
      auto begin = static_cast<Elem *>(&values.begin()->operator()(0));
      auto end = begin + model_t<T>::num_element * values.size();
      elems = element_array<Elem>{begin, end};
    }
    return std::make_unique<model_t<T>>(dimensions.volume(), unit,
                                        std::move(elems));
  }
}

} // namespace

template <class T>
Variable::Variable(const units::Unit unit, const Dimensions &dimensions,
                   T values_, std::optional<T> variances_)
    : m_dims(dimensions), m_strides(dimensions),
      m_object(make_model(unit, dimensions, std::move(values_),
                          std::move(variances_))) {}

template <class T, class... Index>
Variable Variable::elements(const Index &... index) const {
  auto elements(*this);
  const auto &model = cast<T>(*this);
  elements.m_object = model.elements();
  elements.m_offset *= model_t<T>::num_element;
  for (scipp::index i = 0; i < dims().ndim(); ++i)
    elements.m_strides[i] = model_t<T>::num_element * strides()[i];
  if constexpr (sizeof...(index) == 0) {
    Strides inner_strides(inner_dims<T>);
    for (scipp::index i = 0; i < inner_dims<T>.ndim(); ++i)
      elements.unchecked_strides()[elements.dims().ndim() + i] =
          inner_strides[i];
    elements.unchecked_dims() = merge(elements.dims(), inner_dims<T>);
  } else {
    elements.m_offset += model.element_offset(index...);
  }
  return elements;
}

template <class T> ElementArrayView<const T> Variable::values() const {
  return cast<T>(*this).values(array_params());
}
template <class T> ElementArrayView<T> Variable::values() {
  return cast<T>(*this).values(array_params());
}
template <class T> ElementArrayView<const T> Variable::variances() const {
  if constexpr (!core::canHaveVariances<T>())
    throw except::VariancesError("This dtype cannot have variances.");
  else
    return cast<T>(*this).variances(array_params());
}
template <class T> ElementArrayView<T> Variable::variances() {
  if constexpr (!core::canHaveVariances<T>())
    throw except::VariancesError("This dtype cannot have variances.");
  else
    return cast<T>(*this).variances(array_params());
}

#define INSTANTIATE_VARIABLE_BASE(name, ...)                                   \
  namespace {                                                                  \
  auto register_dtype_name_##name(                                             \
      (core::dtypeNameRegistry().emplace(dtype<__VA_ARGS__>, #name), 0));      \
  }                                                                            \
  template SCIPP_EXPORT ElementArrayView<const __VA_ARGS__> Variable::values() \
      const;                                                                   \
  template SCIPP_EXPORT ElementArrayView<__VA_ARGS__> Variable::values();

/// Macro for instantiating classes and functions required for support a new
/// dtype in Variable.
#define INSTANTIATE_VARIABLE(name, ...)                                        \
  INSTANTIATE_VARIABLE_BASE(name, __VA_ARGS__)                                 \
  namespace {                                                                  \
  auto register_variable_maker_##name((                                        \
      variableFactory().emplace(                                               \
          dtype<__VA_ARGS__>, std::make_unique<VariableMaker<__VA_ARGS__>>()), \
      0));                                                                     \
  }                                                                            \
  template SCIPP_EXPORT Variable::Variable(                                    \
      const units::Unit, const Dimensions &, element_array<__VA_ARGS__>,       \
      std::optional<element_array<__VA_ARGS__>>);                              \
  template SCIPP_EXPORT ElementArrayView<const __VA_ARGS__>                    \
  Variable::variances() const;                                                 \
  template SCIPP_EXPORT ElementArrayView<__VA_ARGS__> Variable::variances();

template <class T> std::string Formatter<T>::format(const Variable &var) const {
  return array_to_string(var.template values<T>());
}

/// Insert classes into formatting registry. The objects themselves do nothing,
/// but the constructor call with comma operator does the insertion. Calling
/// this is required for formatting all but basic builtin types.
#define REGISTER_FORMATTER(name, ...)                                          \
  namespace {                                                                  \
  auto register_##name(                                                        \
      (variable::formatterRegistry().emplace(                                  \
           dtype<__VA_ARGS__>,                                                 \
           std::make_unique<variable::Formatter<__VA_ARGS__>>()),              \
       0));                                                                    \
  }

} // namespace scipp::variable
