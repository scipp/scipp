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
#include "scipp/variable/data_model.h"
#include "scipp/variable/except.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_concept.h"

namespace scipp::variable {

/// Implementation of VariableConcept that holds and array with element type T.
template <class T, int... N> class MatrixModel : public VariableConcept {
public:
  using value_type = T;

  MatrixModel(const Variable &elements)
      : VariableConcept(elements.unit()),
        // copy in case this is a slice?
        m_elements(variable::copy(elements).data_handle()) {
    if (hasVariances())
      throw except::VariancesError("Matrix data type cannot have variances.");
    if (elements.dtype() != scipp::dtype<double>)
      throw except::TypeError(
          "Matrix data type only supported with float64 elements.");
    if (elements.dims().volume() % (N * ...) != 0)
      throw except::DimensionError("Underlying elements do not have correct "
                                   "shape for this matrix type.");
  }

  static DType static_dtype() noexcept { return scipp::dtype<T>; }
  DType dtype() const noexcept override { return scipp::dtype<T>; }
  scipp::index size() const override { return m_elements->size() / (N * ...); }

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
    return std::make_unique<MatrixModel<T, N...>>(*this);
  }

  bool hasVariances() const noexcept override {
    return m_elements->hasVariances();
  }

  auto values(const core::ElementArrayViewParams &base) const {
    return ElementArrayView(base, get_values());
  }
  auto values(const core::ElementArrayViewParams &base) {
    return ElementArrayView(base, get_values());
  }

  scipp::index dtype_size() const override { return sizeof(T); }
  const VariableConceptHandle &bin_indices() const override {
    throw except::TypeError("This data type does not have bin indices.");
  }

  scipp::span<const T> values() const { return {get_values(), size()}; }
  scipp::span<T> values() { return {get_values(), size()}; }

private:
  const T *get_values() const {
    return reinterpret_cast<const T *>(
        requireT<const DataModel<double>>(*m_elements).values().data());
  }
  T *get_values() {
    return reinterpret_cast<T *>(
        requireT<DataModel<double>>(*m_elements).values().data());
  }
  VariableConceptHandle m_elements;
};

template <class T, int... N>
VariableConceptHandle
MatrixModel<T, N...>::makeDefaultFromParent(const scipp::index size) const {
  throw std::runtime_error("todo how to get dims?");
  // return std::make_unique<MatrixModel<T, N...>>();
}

/// Helper for implementing Variable(View)::operator==.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// values<T> and variances<T> can be compared.
template <class T, int... N>
bool MatrixModel<T, N...>::equals(const Variable &a, const Variable &b) const {
  if (a.dims() != b.dims())
    return false;
  const auto &a_elems =
      *requireT<const MatrixModel<T, N...>>(a.data()).m_elements;
  const auto &b_elems =
      *requireT<const MatrixModel<T, N...>>(b.data()).m_elements;
  throw std::runtime_error("todo ");
  // return a_elems == b_elems;
}

/// Helper for implementing Variable(View) copy operations.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// transform can be called with any T.
template <class T, int... N>
void MatrixModel<T, N...>::copy(const Variable &src, Variable &dest) const {
  transform_in_place<T>(dest, src, [](auto &a, const auto &b) { a = b; });
}
template <class T, int... N>
void MatrixModel<T, N...>::copy(const Variable &src, Variable &&dest) const {
  copy(src, dest);
}

template <class T, int... N>
void MatrixModel<T, N...>::assign(const VariableConcept &other) {
  *this = requireT<const MatrixModel<T, N...>>(other);
}

template <class T, int... N>
void MatrixModel<T, N...>::setVariances(const Variable &) {
  throw except::VariancesError("This data type cannot have variances.");
}

} // namespace scipp::variable
