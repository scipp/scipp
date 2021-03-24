// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include "scipp/common/initialization.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/units/unit.h"
#include "scipp/variable/except.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_concept.h"
#include <optional>

namespace scipp::variable {

template <class T, class C> auto &requireT(C &varconcept) {
  if (varconcept.dtype() != dtype<typename T::value_type>)
    throw except::TypeError("Expected item dtype " +
                            to_string(T::static_dtype()) + ", got " +
                            to_string(varconcept.dtype()) + '.');
  return static_cast<T &>(varconcept);
}

template <class T> struct is_span : std::false_type {};
template <class T> struct is_span<scipp::span<T>> : std::true_type {};
template <class T> inline constexpr bool is_span_v = is_span<T>::value;

template <class T1, class T2>
bool equals_impl(const T1 &view1, const T2 &view2) {
  // TODO Use optimizations in case of contigous views (instead of slower
  // ElementArrayView iteration). Add multi threading?
  if constexpr (is_span_v<typename T1::value_type>)
    return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end(),
                      [](auto &a, auto &b) { return equals_impl(a, b); });
  else
    return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end());
}

/// Implementation of VariableConcept that holds and array with element type T.
template <class T> class DataModel : public VariableConcept {
public:
  using value_type = T;

  DataModel(const scipp::index size, const units::Unit &unit,
            element_array<T> model,
            std::optional<element_array<T>> variances = std::nullopt)
      : VariableConcept(unit),
        m_values(model ? std::move(model)
                       : element_array<T>(size, default_init<T>::value())),
        m_variances(std::move(variances)) {
    if (m_variances && !core::canHaveVariances<T>())
      throw except::VariancesError("This data type cannot have variances.");
    if (size != scipp::size(m_values))
      throw except::DimensionError(
          "Creating Variable: data size does not match "
          "volume given by dimension extents.");
    if (m_variances && !*m_variances)
      *m_variances = element_array<T>(size, default_init<T>::value());
  }

  static DType static_dtype() noexcept { return scipp::dtype<T>; }
  DType dtype() const noexcept override { return scipp::dtype<T>; }
  scipp::index size() const override { return m_values.size(); }

  VariableConceptHandle
  makeDefaultFromParent(const scipp::index size) const override;

  VariableConceptHandle
  makeDefaultFromParent(const Variable &shape) const override {
    return makeDefaultFromParent(shape.dims().volume());
  }

  bool equals(const Variable &a, const Variable &b) const override;
  void copy(const Variable &src, Variable &dest) const override;
  void copy(const Variable &src, Variable &&dest) const override;
  void assign(const VariableConcept &other) override;

  void setVariances(const Variable &variances) override;

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel<T>>(*this);
  }

  bool hasVariances() const noexcept override {
    return m_variances.has_value();
  }

  auto values(const core::ElementArrayViewParams &base) const {
    return ElementArrayView(base, m_values.data());
  }
  auto values(const core::ElementArrayViewParams &base) {
    return ElementArrayView(base, m_values.data());
  }
  auto variances(const core::ElementArrayViewParams &base) const {
    expectHasVariances();
    return ElementArrayView(base, m_variances->data());
  }
  auto variances(const core::ElementArrayViewParams &base) {
    expectHasVariances();
    return ElementArrayView(base, m_variances->data());
  }

  scipp::index dtype_size() const override { return sizeof(T); }
  const VariableConceptHandle &bin_indices() const override {
    throw except::TypeError("This data type does not have bin indices.");
  }

  scipp::span<const T> values() const {
    return {m_values.data(), m_values.data() + m_values.size()};
  }

  scipp::span<T> values() {
    return {m_values.data(), m_values.data() + m_values.size()};
  }

private:
  void expectHasVariances() const {
    if (!hasVariances())
      throw except::VariancesError("Variable does not have variances.");
  }
  element_array<T> m_values;
  std::optional<element_array<T>> m_variances;
};

template <class T> const DataModel<T> &cast(const Variable &var) {
  return requireT<const DataModel<T>>(var.data());
}

template <class T> DataModel<T> &cast(Variable &var) {
  return requireT<DataModel<T>>(var.data());
}

template <class T>
VariableConceptHandle
DataModel<T>::makeDefaultFromParent(const scipp::index size) const {
  if (hasVariances())
    return std::make_unique<DataModel<T>>(size, unit(), element_array<T>(size),
                                          element_array<T>(size));
  else
    return std::make_unique<DataModel<T>>(size, unit(), element_array<T>(size));
}

/// Helper for implementing Variable(View)::operator==.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// values<T> and variances<T> can be compared.
template <class T>
bool DataModel<T>::equals(const Variable &a, const Variable &b) const {
  if (a.unit() != b.unit())
    return false;
  if (a.dims() != b.dims())
    return false;
  if (a.dtype() != b.dtype())
    return false;
  if (a.hasVariances() != b.hasVariances())
    return false;
  if (a.dims().volume() == 0 && a.dims() == b.dims())
    return true;
  return equals_impl(a.values<T>(), b.values<T>()) &&
         (!a.hasVariances() || equals_impl(a.variances<T>(), b.variances<T>()));
}

/// Helper for implementing Variable(View) copy operations.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// transform can be called with any T.
template <class T>
void DataModel<T>::copy(const Variable &src, Variable &dest) const {
  transform_in_place<T>(
      dest, src,
      overloaded{core::transform_flags::expect_in_variance_if_out_variance,
                 [](auto &a, const auto &b) { a = b; }});
}
template <class T>
void DataModel<T>::copy(const Variable &src, Variable &&dest) const {
  copy(src, dest);
}

template <class T> void DataModel<T>::assign(const VariableConcept &other) {
  *this = requireT<const DataModel<T>>(other);
}

template <class T> void DataModel<T>::setVariances(const Variable &variances) {
  if (!core::canHaveVariances<T>())
    throw except::VariancesError("This data type cannot have variances.");
  if (!variances)
    return m_variances.reset();
  // TODO Could move if refcount is 1?
  if (variances.hasVariances())
    throw except::VariancesError(
        "Cannot set variances from variable with variances.");
  m_variances.emplace(requireT<const DataModel>(variances.data()).m_values);
}

} // namespace scipp::variable
