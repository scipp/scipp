// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/element_array_model.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

template <class T>
ElementArrayModel<T>::ElementArrayModel(
    const scipp::index size, const units::Unit &unit, element_array<T> model,
    std::optional<element_array<T>> variances)
    : VariableConcept(unit),
      m_values(model ? std::move(model)
                     : element_array<T>(size, default_init<T>::value())),
      m_variances(std::move(variances)) {
  if (m_variances)
    core::expect::canHaveVariances<T>();
  if (size != scipp::size(m_values))
    throw except::DimensionError("Creating Variable: data size does not match "
                                 "volume given by dimension extents.");
  if (m_variances && !*m_variances)
    *m_variances = element_array<T>(size, default_init<T>::value());
}

template <class T>
void ElementArrayModel<T>::assign(const VariableConcept &other) {
  *this = requireT<const ElementArrayModel<T>>(other);
}

template <class T>
void ElementArrayModel<T>::setVariances(const Variable &variances) {
  if (!core::canHaveVariances<T>())
    throw except::VariancesError("This data type cannot have variances.");
  if (!variances.is_valid())
    return m_variances.reset();
  // TODO Could move if refcount is 1?
  if (variances.hasVariances())
    throw except::VariancesError(
        "Cannot set variances from variable with variances.");
  m_variances.emplace(
      requireT<const ElementArrayModel>(variances.data()).m_values);
}

/// Macro for instantiating classes and functions required for support a new
/// dtype in Variable.
#define INSTANTIATE_ELEMENT_ARRAY_VARIABLE(name, ...)                          \
  template class SCIPP_EXPORT ElementArrayModel<__VA_ARGS__>;                  \
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

} // namespace scipp::variable
