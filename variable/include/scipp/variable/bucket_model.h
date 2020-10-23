// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <algorithm>

#include "scipp/core/bucket_array_view.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/variable/data_model.h"
#include "scipp/variable/except.h"

namespace scipp::variable {

/// Specialization of DataModel for "bucketed" data. T could be Variable,
/// DataArray, or Dataset.
///
/// A bucket in this context is defined as an element of a variable mapping to a
/// range of data, such as a slice of a DataArray.
template <class T> class DataModel<bucket<T>> : public VariableConcept {
public:
  using value_type = bucket<T>;
  using range_type = typename bucket<T>::range_type;

  DataModel(const VariableConstView &indices, const Dim dim, T buffer)
      : VariableConcept(indices.dims()),
        m_indices(validated_indices(indices, dim, buffer)), m_dim(dim),
        m_buffer(std::move(buffer)) {}

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel>(*this);
  }

  bool operator==(const DataModel &other) const noexcept {
    return dims() == other.dims() && m_indices == other.m_indices &&
           m_dim == other.m_dim && m_buffer == other.m_buffer;
  }
  bool operator!=(const DataModel &other) const noexcept {
    return !(*this == other);
  }

  VariableConceptHandle
  makeDefaultFromParent(const Dimensions &dims) const override {
    return std::make_unique<DataModel>(makeVariable<range_type>(dims), m_dim,
                                       T{m_buffer.slice({m_dim, 0, 0})});
  }

  static DType static_dtype() noexcept { return scipp::dtype<bucket<T>>; }
  DType dtype() const noexcept override { return scipp::dtype<bucket<T>>; }

  bool hasVariances() const noexcept override { return false; }
  void setVariances(Variable &&) override {
    if (!core::canHaveVariances<T>())
      throw except::VariancesError("This data type cannot have variances.");
  }

  bool equals(const VariableConstView &a,
              const VariableConstView &b) const override;
  void copy(const VariableConstView &src,
            const VariableView &dest) const override;
  void assign(const VariableConcept &other) override;

  Dim dim() const noexcept { return m_dim; }
  // TODO Should the mutable version return a view to prevent risk of clients
  // breaking invariants of variable?
  const T &buffer() const noexcept { return m_buffer; }
  T &buffer() noexcept { return m_buffer; }
  const Variable &indices() const { return m_indices; }
  Variable &indices() { return m_indices; }

  ElementArrayView<bucket<T>> values(const core::element_array_view &base) {
    return {index_values(base), m_dim, m_buffer};
  }
  ElementArrayView<const bucket<T>>
  values(const core::element_array_view &base) const {
    return {index_values(base), m_dim, m_buffer};
  }

  scipp::index dtype_size() const override { return sizeof(range_type); }

private:
  static auto validated_indices(const VariableConstView &indices, const Dim dim,
                                const T &buffer) {
    Variable copy(indices);
    const auto vals =
        scipp::span(copy.values<range_type>().data(),
                    copy.values<range_type>().data() + copy.dims().volume());
    std::sort(vals.begin(), vals.end());
    if ((!vals.empty() && (vals.begin()->first < 0)) ||
        (!vals.empty() && ((vals.end() - 1)->second > buffer.dims()[dim])))
      throw except::SliceError("Bucket indices out of range");
    if (std::adjacent_find(vals.begin(), vals.end(),
                           [](const auto a, const auto b) {
                             return a.second > b.first;
                           }) != vals.end())
      throw except::SliceError(
          "Bucket begin index must be less or equal to its end index.");
    if (std::find_if(vals.begin(), vals.end(), [](const auto x) {
          return x.second >= 0 && x.first > x.second;
        }) != vals.end())
      throw except::SliceError("Overlapping bucket indices are not allowed.");
    // Copy to avoid a second memory allocation
    const auto &i = indices.values<range_type>();
    std::copy(i.begin(), i.end(), vals.begin());
    return copy;
  }
  auto index_values(const core::element_array_view &base) const {
    return cast<range_type>(m_indices).values(base);
  }
  Variable m_indices;
  Dim m_dim;
  T m_buffer;
};

template <class T>
bool DataModel<bucket<T>>::equals(const VariableConstView &a,
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
  // TODO This implementation is slow since it creates a view for every bucket.
  return equal(a.values<bucket<T>>(), b.values<bucket<T>>());
}

template <class T>
void DataModel<bucket<T>>::copy(const VariableConstView &,
                                const VariableView &) const {
  // Need to rethink the entire mechanism of shape-changing operations such as
  // `concatenate` since we cannot just copy indices but need to manage the
  // buffer as well.
  throw std::runtime_error(
      "Shape-related operations for bucketed data are not supported yet.");
}

template <class T>
void DataModel<bucket<T>>::assign(const VariableConcept &other) {
  *this = requireT<const DataModel<bucket<T>>>(other);
}

} // namespace scipp::variable
