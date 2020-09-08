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

  DataModel(const Dimensions &dimensions,
            const element_array<range_type> &indices, const Dim dim, T buffer)
      : VariableConcept(dimensions), m_dim(dim), m_buffer(std::move(buffer)),
        m_indices(validated_indices(indices)) {
    if (this->dims().volume() != scipp::size(m_indices))
      throw except::DimensionError(
          "Creating Variable: data size does not match "
          "volume given by dimension extents.");
  }

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel>(this->dims(), m_indices, m_dim,
                                       m_buffer);
  }

  bool operator==(const DataModel &other) const noexcept {
    return dims() == other.dims() && equal(m_indices, other.m_indices) &&
           m_dim == other.m_dim && m_buffer == other.m_buffer;
  }
  bool operator!=(const DataModel &other) const noexcept {
    return !(*this == other);
  }

  VariableConceptHandle
  makeDefaultFromParent(const Dimensions &dims) const override {
    return std::make_unique<DataModel>(dims,
                                       element_array<range_type>(dims.volume()),
                                       m_dim, T{m_buffer.slice({m_dim, 0, 0})});
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

  auto indices() const {
    return ElementArrayView{m_indices.data(), 0, dims(), dims()};
  }
  auto indices(const scipp::index offset, const Dimensions &iterDims,
               const Dimensions &dataDims) const {
    return ElementArrayView(m_indices.data(), offset, iterDims, dataDims);
  }

  ElementArrayView<bucket<T>> values() { return {indices(), m_dim, m_buffer}; }
  ElementArrayView<const bucket<T>> values() const {
    return {indices(), m_dim, m_buffer};
  }

  ElementArrayView<bucket<T>> values(const scipp::index offset,
                                     const Dimensions &iterDims,
                                     const Dimensions &dataDims) {
    return {indices(offset, iterDims, dataDims), m_dim, m_buffer};
  }
  ElementArrayView<const bucket<T>> values(const scipp::index offset,
                                           const Dimensions &iterDims,
                                           const Dimensions &dataDims) const {
    return {indices(offset, iterDims, dataDims), m_dim, m_buffer};
  }

private:
  auto validated_indices(const element_array<range_type> &indices) {
    auto copy(indices);
    std::sort(copy.begin(), copy.end());
    if ((!copy.empty() && (copy.begin()->first < 0)) ||
        (!copy.empty() && ((copy.end() - 1)->second > m_buffer.dims()[m_dim])))
      throw except::SliceError("Bucket indices out of range");
    if (std::adjacent_find(copy.begin(), copy.end(),
                           [](const auto a, const auto b) {
                             return a.second > b.first;
                           }) != copy.end())
      throw except::SliceError(
          "Bucket begin index must be less or equal to its end index.");
    if (std::find_if(copy.begin(), copy.end(), [](const auto x) {
          return x.second >= 0 && x.first > x.second;
        }) != copy.end())
      throw except::SliceError("Overlapping bucket indices are not allowed.");
    // Copy to avoid a second memory allocation
    std::copy(indices.begin(), indices.end(), copy.begin());
    return copy;
  }
  Dim m_dim;
  T m_buffer;
  element_array<range_type> m_indices;
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
  const auto &otherT = requireT<const DataModel<bucket<T>>>(other);
  std::copy(otherT.m_indices.begin(), otherT.m_indices.end(),
            m_indices.begin());
  m_dim = otherT.m_dim;
  m_buffer = otherT.m_buffer;
}

} // namespace scipp::variable
