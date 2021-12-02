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
#include "scipp/variable/element_array_model.h"
#include "scipp/variable/except.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

template <class T> struct model { using type = ElementArrayModel<T>; };
template <class T> using model_t = typename model<T>::type;

namespace {

template <class T, class C> auto &requireT(C &varconcept) {
  if (varconcept.dtype() != dtype<typename T::value_type>)
    throw except::TypeError("Expected item dtype " +
                            to_string(T::static_dtype()) + ", got " +
                            to_string(varconcept.dtype()) + '.');
  return static_cast<T &>(varconcept);
}

template <class T> const auto &cast(const Variable &var) {
  return requireT<const model_t<T>>(var.data());
}

template <class T> auto &cast(Variable &var) {
  return requireT<model_t<T>>(var.data());
}

template <int I, class T> decltype(auto) get(T &&t) {
  if constexpr (std::is_same_v<std::decay_t<T>, Eigen::Affine3d>) {
    return t.matrix().operator()(I);
  } else if constexpr (core::has_eval_v<std::decay_t<T>> ||
                       std::is_same_v<std::decay_t<T>,
                                      scipp::core::Quaternion> ||
                       std::is_same_v<std::decay_t<T>,
                                      scipp::core::Translation>) {
    return t.operator()(I);
  } else {
    return std::get<I>(t);
  }
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
      auto begin = static_cast<Elem *>(&get<0>(*values.begin()));
      auto end = begin + model_t<T>::element_count * values.size();
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

template <class T> ElementArrayView<const T> Variable::values() const {
  return cast<T>(*this).values(array_params());
}
template <class T> ElementArrayView<T> Variable::values() {
  return cast<T>(*this).values(array_params());
}
template <class T> ElementArrayView<const T> Variable::variances() const {
  if constexpr (!core::canHaveVariances<T>())
    except::throw_cannot_have_variances(core::dtype<T>);
  else
    return cast<T>(*this).variances(array_params());
}
template <class T> ElementArrayView<T> Variable::variances() {
  if constexpr (!core::canHaveVariances<T>())
    except::throw_cannot_have_variances(core::dtype<T>);
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
