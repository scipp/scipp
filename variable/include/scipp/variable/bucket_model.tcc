// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/variable/except.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

template <class T> struct bucket {
  using element_type = typename T::view_type;
  using const_element_type = typename T::const_view_type;
};

template <class T> class DataModel<bucket<T>> : public VariableConcept {
public:
  DataModel(const Dimensions &dimensions,
            element_array<std::pair<scipp::index, scipp::index>> buckets,
            const Dim dim, T buffer)
      : VariableConcept(dimensions), m_buckets(std::move(buckets)), m_dim(dim),
        m_buffer(std::move(buffer)) {
    if (!m_buffer.dims().contains(m_dim))
      throw except::DimensionError("Buffer must contain bucket slicing dim.");
  }

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel>(this->dims(), m_buckets, m_dim,
                                       m_buffer);
  }

  bool operator==(const DataModel &other) const noexcept {
    return dims() == other.dims() && equal(m_buckets, other.m_buckets) &&
           m_dim == other.m_dim && m_buffer == other.m_buffer;
  }
  bool operator!=(const DataModel &other) const noexcept {
    return !(*this == other);
  }

  VariableConceptHandle
  makeDefaultFromParent(const Dimensions &dims) const override {
    return std::make_unique<DataModel>(
        dims,
        element_array<std::pair<scipp::index, scipp::index>>(dims.volume()),
        m_dim, T(m_buffer, m_buffer.dims()));
  }

  constexpr DType dtype() const noexcept override {
    return scipp::dtype<bucket<T>>;
  }

  constexpr bool hasVariances() const noexcept override { return false; }
  void setVariances(Variable &&variances) override {
    if (!core::canHaveVariances<T>())
      throw except::VariancesError("This data type cannot have variances.");
  }

  bool equals(const VariableConstView &a,
              const VariableConstView &b) const override {
    return false;
  }
  void copy(const VariableConstView &src,
            const VariableView &dest) const override {}
  void assign(const VariableConcept &other) override {}

  ElementArrayView<typename bucket<T>::element_type> values();
  ElementArrayView<typename bucket<T>::const_element_type> values() const;

private:
  element_array<std::pair<scipp::index, scipp::index>> m_buckets;
  Dim m_dim;
  T m_buffer;
};

} // namespace scipp::variable
