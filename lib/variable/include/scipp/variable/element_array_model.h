// SPDX-License-Identifier: BSD-3-Clause
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

template <class T> struct is_span : std::false_type {};
template <class T> struct is_span<scipp::span<T>> : std::true_type {};
template <class T> inline constexpr bool is_span_v = is_span<T>::value;

template <class T1, class T2>
bool equals_impl(const T1 &view1, const T2 &view2) {
  // TODO Use optimizations in case of contigous views (instead of slower
  // ElementArrayView iteration). Add multi threading?
  if constexpr (is_span_v<typename T1::value_type>)
    return std::equal(
        view1.begin(), view1.end(), view2.begin(), view2.end(),
        [](const auto &a, const auto &b) { return equals_impl(a, b); });
  else
    return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end());
}

/// Implementation of VariableConcept that holds an array with element type T.
template <class T> class ElementArrayModel : public VariableConcept {
public:
  using value_type = T;

  ElementArrayModel(const scipp::index size, const units::Unit &unit,
                    element_array<T> model,
                    std::optional<element_array<T>> variances = std::nullopt);

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

  VariableConceptHandle clone() const override;

  bool has_variances() const noexcept override {
    return m_variances.has_value();
  }

  auto values(const core::ElementArrayViewParams &base) const {
    return ElementArrayView(base, m_values.data());
  }
  auto values(const core::ElementArrayViewParams &base) {
    return ElementArrayView(base, m_values.data());
  }
  auto variances(const core::ElementArrayViewParams &base) const {
    expect_has_variances();
    return ElementArrayView(base, m_variances->data());
  }
  auto variances(const core::ElementArrayViewParams &base) {
    expect_has_variances();
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
  void expect_has_variances() const {
    if (!has_variances())
      throw except::VariancesError("Variable does not have variances.");
  }
  element_array<T> m_values;
  std::optional<element_array<T>> m_variances;
};

namespace {
template <class T> auto copy(const T &x) { return x; }
constexpr auto do_copy = [](auto &a, const auto &b) { a = copy(b); };
} // namespace

/// Helper for implementing Variable(View) copy operations.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// transform can be called with any T.
template <class T>
void ElementArrayModel<T>::copy(const Variable &src, Variable &dest) const {
  transform_in_place<T>(
      dest, src,
      overloaded{core::transform_flags::expect_in_variance_if_out_variance,
                 do_copy},
      "copy");
}
template <class T>
void ElementArrayModel<T>::copy(const Variable &src, Variable &&dest) const {
  copy(src, dest);
}

} // namespace scipp::variable
