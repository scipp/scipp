// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/units/unit.h"
#include "scipp/variable/except.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_concept.h"
#include <optional>

namespace scipp::variable {

template <class T, class C> auto &requireT(C &concept) {
  if (concept.dtype() != dtype<typename T::value_type>)
    throw except::TypeError("Expected item dtype " +
                            to_string(T::static_dtype()) + ", got " +
                            to_string(concept.dtype()) + '.');
  return static_cast<T &>(concept);
}

template <class T> struct is_span : std::false_type {};
template <class T> struct is_span<scipp::span<T>> : std::true_type {};
template <class T> inline constexpr bool is_span_v = is_span<T>::value;

template <class T1, class T2> bool equal(const T1 &view1, const T2 &view2) {
  // TODO Use optimizations in case of contigous views (instead of slower
  // ElementArrayView iteration). Add multi threading?
  if constexpr (is_span_v<typename T1::value_type>)
    return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end(),
                      [](auto &a, auto &b) { return equal(a, b); });
  else
    return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end());
}

namespace {
template <class T> struct default_init {
  static T value() { return T(); }
};
// Eigen does not zero-initialize matrices (vectors), which is a recurrent
// source of bugs. Variable does zero-init instead.
template <class T, int Rows, int Cols>
struct default_init<Eigen::Matrix<T, Rows, Cols>> {
  static Eigen::Matrix<T, Rows, Cols> value() {
    return Eigen::Matrix<T, Rows, Cols>::Zero();
  }
};
} // namespace

/// Implementation of VariableConcept that holds and array with element type T.
template <class T> class DataModel : public VariableConcept {
public:
  using value_type = T;

  DataModel(const Dimensions &dimensions, element_array<T> model,
            std::optional<element_array<T>> variances = std::nullopt)
      : VariableConcept(dimensions),
        m_values(model ? std::move(model)
                       : element_array<T>(dimensions.volume(),
                                          default_init<T>::value())),
        m_variances(std::move(variances)) {
    if (m_variances && !core::canHaveVariances<T>())
      throw except::VariancesError("This data type cannot have variances.");
    if (this->dims().volume() != scipp::size(m_values))
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
    if (m_variances && !*m_variances)
      *m_variances =
          element_array<T>(dimensions.volume(), default_init<T>::value());
  }

  static DType static_dtype() noexcept { return scipp::dtype<T>; }
  DType dtype() const noexcept override { return scipp::dtype<T>; }

  VariableConceptHandle
  makeDefaultFromParent(const Dimensions &dims) const override;

  bool equals(const VariableConstView &a,
              const VariableConstView &b) const override;
  void copy(const VariableConstView &src,
            const VariableView &dest) const override;
  void assign(const VariableConcept &other) override;

  void setVariances(Variable &&variances) override;

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel<T>>(this->dims(), m_values, m_variances);
  }

  bool hasVariances() const noexcept override {
    return m_variances.has_value();
  }

  auto values() const {
    return ElementArrayView(m_values.data(), 0, dims(), dims());
  }
  auto values() { return ElementArrayView(m_values.data(), 0, dims(), dims()); }
  auto variances() const {
    expectHasVariances();
    return ElementArrayView(m_variances->data(), 0, dims(), dims());
  }
  auto variances() {
    expectHasVariances();
    return ElementArrayView(m_variances->data(), 0, dims(), dims());
  }

  auto values(const scipp::index offset, const Dimensions &iterDims,
              const Dimensions &dataDims) const {
    return ElementArrayView(m_values.data(), offset, iterDims, dataDims);
  }
  auto values(const scipp::index offset, const Dimensions &iterDims,
              const Dimensions &dataDims) {
    return ElementArrayView(m_values.data(), offset, iterDims, dataDims);
  }
  auto variances(const scipp::index offset, const Dimensions &iterDims,
                 const Dimensions &dataDims) const {
    expectHasVariances();
    return ElementArrayView(m_variances->data(), offset, iterDims, dataDims);
  }
  auto variances(const scipp::index offset, const Dimensions &iterDims,
                 const Dimensions &dataDims) {
    expectHasVariances();
    return ElementArrayView(m_variances->data(), offset, iterDims, dataDims);
  }

private:
  void expectHasVariances() const {
    if (!hasVariances())
      throw except::VariancesError("Variable does not have variances.");
  }
  element_array<T> m_values;
  std::optional<element_array<T>> m_variances;
};

template <class T>
VariableConceptHandle
DataModel<T>::makeDefaultFromParent(const Dimensions &dims) const {
  if (hasVariances())
    return std::make_unique<DataModel<T>>(dims, element_array<T>(dims.volume()),
                                          element_array<T>(dims.volume()));
  else
    return std::make_unique<DataModel<T>>(dims,
                                          element_array<T>(dims.volume()));
}

/// Helper for implementing Variable(View)::operator==.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// values<T> and variances<T> can be compared.
template <class T>
bool DataModel<T>::equals(const VariableConstView &a,
                          const VariableConstView &b) const {
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
  return equal(a.values<T>(), b.values<T>()) &&
         (!a.hasVariances() || equal(a.variances<T>(), b.variances<T>()));
}

/// Helper for implementing Variable(View) copy operations.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// transform can be called with any T.
template <class T>
void DataModel<T>::copy(const VariableConstView &src,
                        const VariableView &dest) const {
  transform_in_place<T>(
      dest, src,
      overloaded{core::transform_flags::no_event_list_handling,
                 core::transform_flags::expect_in_variance_if_out_variance,
                 [](auto &a, const auto &b) { a = b; }});
}

template <class T> void DataModel<T>::assign(const VariableConcept &other) {
  const auto &otherT = requireT<const DataModel<T>>(other);
  std::copy(otherT.m_values.begin(), otherT.m_values.end(), m_values.begin());
  if (hasVariances())
    std::copy(otherT.m_variances->begin(), otherT.m_variances->end(),
              m_variances->begin());
}

template <class T> void DataModel<T>::setVariances(Variable &&variances) {
  if (!core::canHaveVariances<T>())
    throw except::VariancesError("This data type cannot have variances.");
  if (!variances)
    return m_variances.reset();
  if (variances.hasVariances())
    throw except::VariancesError(
        "Cannot set variances from variable with variances.");
  core::expect::equals(this->dims(), variances.dims());
  m_variances.emplace(
      std::move(requireT<DataModel>(variances.data()).m_values));
}

} // namespace scipp::variable
