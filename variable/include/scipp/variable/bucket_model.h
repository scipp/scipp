// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include "scipp/core/bucket_array_view.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/variable/except.h"
#include "scipp/variable/data_model.h"

namespace scipp::variable {

/// Specialization of DataModel for "bucketed" data. T could be Variable,
/// DataArray, or Dataset.
template <class T> class DataModel<bucket<T>> : public VariableConcept {
public:
  using value_type = bucket<T>;
  using range_type = typename bucket<T>::range_type;

  DataModel(const Dimensions &dimensions, element_array<range_type> buckets,
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
    return std::make_unique<DataModel>(dims,
                                       element_array<range_type>(dims.volume()),
                                       m_dim, T(m_buffer, m_buffer.dims()));
  }

  static DType static_dtype() noexcept { return scipp::dtype<bucket<T>>; }
  DType dtype() const noexcept override { return scipp::dtype<bucket<T>>; }

  constexpr bool hasVariances() const noexcept override { return false; }
  void setVariances(Variable &&) override {
    if (!core::canHaveVariances<T>())
      throw except::VariancesError("This data type cannot have variances.");
  }

  bool equals(const VariableConstView &,
              const VariableConstView &) const override {
    return false;
  }
  void copy(const VariableConstView &, const VariableView &) const override {}
  void assign(const VariableConcept &) override {}

  auto buckets() const {
    return ElementArrayView{m_buckets.data(), 0, dims(), dims()};
  }

  ElementArrayView<bucket<T>> values() { return {buckets(), m_dim, m_buffer}; }
  ElementArrayView<const bucket<T>> values() const {
    return {buckets(), m_dim, m_buffer};
  }

private:
  element_array<range_type> m_buckets;
  Dim m_dim;
  T m_buffer;
};

#define INSTANTIATE_BUCKET_VARIABLE(name, ...)                                 \
  namespace {                                                                  \
  auto register_dtype_name_##name(                                             \
      (core::dtypeNameRegistry().emplace(dtype<__VA_ARGS__>, #name), 0));      \
  }

} // namespace scipp::variable
