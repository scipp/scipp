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
#include "scipp/variable/bin_array_model.h"
#include "scipp/variable/element_array_model.h"
#include "scipp/variable/except.h"
#include "scipp/variable/structure_array_model.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

template <class T> struct model { using type = ElementArrayModel<T>; };
template <class T> using model_t = typename model<T>::type;

namespace {

template <class T> const auto &cast(const Variable &var) {
  return requireT<const model_t<T>>(var.data());
}

template <class T> auto &cast(Variable &var) {
  return requireT<model_t<T>>(var.data());
}

template <class T>
auto make_model(const units::Unit unit, const Dimensions &dimensions,
                element_array<T> values,
                std::optional<element_array<T>> variances) {
  if constexpr (std::is_same_v<model_t<T>, ElementArrayModel<T>>) {
    return std::make_unique<model_t<T>>(
        dimensions.volume(), unit, std::move(values), std::move(variances));
  } else {
    // There is an extra copy caused here, but in practice this constructor
    // should not be used much outside unit tests.
    using Elem = typename model_t<T>::element_type;
    element_array<Elem> elems;
    if (values) {
      auto begin = static_cast<Elem *>(&values.begin()->operator()(0));
      auto end = begin + model_t<T>::element_count * values.size();
      elems = element_array<Elem>{begin, end};
    }
    return std::make_unique<model_t<T>>(dimensions.volume(), unit,
                                        std::move(elems));
  }
}

} // namespace

template <class T>
Variable make_default_init(const Dimensions &dims, const units::Unit &unit,
                           const bool variances) {
  if (variances && !core::canHaveVariances<T>())
    throw except::VariancesError("This data type cannot have variances.");
  const auto volume = dims.volume();
  VariableConceptHandle model;
  if constexpr (std::is_same_v<model_t<T>, ElementArrayModel<T>>) {
    if (variances)
      model = std::make_shared<model_t<T>>(
          volume, unit, element_array<T>(volume, core::default_init_elements),
          element_array<T>(volume, core::default_init_elements));
    else
      model = std::make_shared<model_t<T>>(
          volume, unit, element_array<T>(volume, core::default_init_elements));
  } else {
    using Elem = typename model_t<T>::element_type;
    model = std::make_shared<model_t<T>>(
        volume, unit,
        element_array<Elem>(model_t<T>::element_count * volume,
                            core::default_init_elements));
  }
  return Variable(dims, std::move(model));
}

template <class T>
Variable::Variable(const units::Unit unit, const Dimensions &dimensions,
                   T values_, std::optional<T> variances_)
    : m_dims(dimensions), m_strides(dimensions),
      m_object(make_model(unit, dimensions, std::move(values_),
                          std::move(variances_))) {}

template <class T>
constexpr auto structure_element_offset{
    T::missing_specialization_of_structure_element_offset};

template <class T, class... Is> Variable Variable::elements(Is... index) const {
  constexpr auto N = model_t<T>::element_count;
  auto elements(*this);
  elements.m_object = cast<T>(*this).elements();
  // Scale offset and strides (which refer to type T) so they are correct for
  // the *element type* of T.
  elements.m_offset *= N;
  for (scipp::index i = 0; i < dims().ndim(); ++i)
    elements.m_strides[i] = N * strides()[i];
  if constexpr (sizeof...(Is) == 0) {
    // Get all elements by setting up internal dim and stride
    elements.unchecked_dims().addInner(Dim::Internal0, N);
    elements.unchecked_strides()[dims().ndim()] = 1;
  } else {
    // Get specific element at offset
    elements.m_offset += structure_element_offset<T>(index...);
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

template <class T> struct arg_type;
template <class T, class U> struct arg_type<T(U)> { using type = U; };

/// Macro for instantiating classes and functions required for support a new
/// dtype in Variable.
#define INSTANTIATE_VARIABLE(name, ...)                                        \
  template SCIPP_EXPORT Variable variable::make_default_init<__VA_ARGS__>(     \
      const Dimensions &, const units::Unit &, const bool);                    \
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

/// Instantiate Variable for structure dtype with element access.
#define INSTANTIATE_STRUCTURE_VARIABLE(name, T, Elem, ...)                     \
  template <> struct model<arg_type<void(T)>::type> {                          \
    using type = StructureArrayModel<arg_type<void(T)>::type, Elem>;           \
  };                                                                           \
  template SCIPP_EXPORT Variable Variable::elements<arg_type<void(T)>::type>() \
      const;                                                                   \
  template SCIPP_EXPORT Variable Variable::elements<arg_type<void(T)>::type>(  \
      __VA_ARGS__) const;                                                      \
  INSTANTIATE_VARIABLE(name, arg_type<void(T)>::type)

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
