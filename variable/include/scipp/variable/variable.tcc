// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/units/unit.h"
#include "scipp/variable/data_model.h"
#include "scipp/variable/except.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

template <class T>
Variable::Variable(const units::Unit unit, const Dimensions &dimensions,
                   T values_, std::optional<T> variances_)
    : m_unit{unit},
      m_object(std::make_unique<DataModel<typename T::value_type>>(
          std::move(dimensions), std::move(values_), std::move(variances_))) {}

template <class T> ElementArrayView<const T> Variable::values() const {
  return cast<T>(*this).values(array_params());
}
template <class T> ElementArrayView<T> Variable::values() {
  return cast<T>(*this).values(array_params());
}
template <class T> ElementArrayView<const T> Variable::variances() const {
  return cast<T>(*this).variances(array_params());
}
template <class T> ElementArrayView<T> Variable::variances() {
  return cast<T>(*this).variances(array_params());
}

template <class T> ElementArrayView<const T> VariableConstView::values() const {
  return cast<T>(*m_variable).values(array_params());
}
template <class T>
ElementArrayView<const T> VariableConstView::variances() const {
  return cast<T>(*m_variable).variances(array_params());
}

template <class T> ElementArrayView<T> VariableView::values() const {
  return cast<T>(*m_mutableVariable).values(array_params());
}
template <class T> ElementArrayView<T> VariableView::variances() const {
  return cast<T>(*m_mutableVariable).variances(array_params());
}

template <class T> void VariableView::replace_model(T model) const {
  core::expect::equals(dims(),
                       m_mutableVariable->dims()); // trivial view (no slice)
  core::expect::equals(dims(),
                       model.dims()); // shape change would break DataArray
  requireT<T>(m_mutableVariable->data()) = std::move(model);
}

#define INSTANTIATE_VARIABLE_BASE(name, ...)                                   \
  namespace {                                                                  \
  auto register_dtype_name_##name(                                             \
      (core::dtypeNameRegistry().emplace(dtype<__VA_ARGS__>, #name), 0));      \
  }                                                                            \
  template ElementArrayView<const __VA_ARGS__> Variable::values() const;       \
  template ElementArrayView<__VA_ARGS__> Variable::values();                   \
  template ElementArrayView<const __VA_ARGS__> VariableConstView::values()     \
      const;                                                                   \
  template ElementArrayView<__VA_ARGS__> VariableView::values() const;         \
  template void VariableView::replace_model<DataModel<__VA_ARGS__>>(           \
      DataModel<__VA_ARGS__>) const;

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
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              element_array<__VA_ARGS__>,                      \
                              std::optional<element_array<__VA_ARGS__>>);      \
  template ElementArrayView<const __VA_ARGS__> Variable::variances() const;    \
  template ElementArrayView<__VA_ARGS__> Variable::variances();                \
  template ElementArrayView<const __VA_ARGS__> VariableConstView::variances()  \
      const;                                                                   \
  template ElementArrayView<__VA_ARGS__> VariableView::variances() const;

} // namespace scipp::variable
