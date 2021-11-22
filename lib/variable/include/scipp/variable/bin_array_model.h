// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <algorithm>

#include "scipp/core/bucket_array_view.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/element_array_model.h"
#include "scipp/variable/except.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"

namespace scipp::variable {

template <class Indices> class BinModelBase : public VariableConcept {
public:
  BinModelBase(const VariableConceptHandle &indices, const Dim dim)
      : VariableConcept(units::one), m_indices(indices), m_dim(dim) {}

  scipp::index size() const override { return indices()->size(); }

  void setUnit(const units::Unit &unit) override {
    if (unit != units::one)
      throw except::UnitError(
          "Bins cannot have a unit. Did you mean to set the unit of the bin "
          "elements? This can be set, e.g., with `array.bins.unit = 'm'`.");
  }

  bool has_variances() const noexcept override { return false; }
  const Indices &bin_indices() const override { return indices(); }

  const auto &indices() const { return m_indices; }
  auto &indices() { return m_indices; }
  Dim bin_dim() const noexcept { return m_dim; }

private:
  Indices m_indices;
  Dim m_dim;
};

/// Specialization of ElementArrayModel for "binned" data. T could be Variable,
/// DataArray, or Dataset.
///
/// A bin in this context is defined as an element of a variable mapping to a
/// range of data, such as a slice of a DataArray.
template <class T>
class BinArrayModel : public BinModelBase<VariableConceptHandle> {
  using Indices = VariableConceptHandle;

public:
  using value_type = bucket<T>;

  BinArrayModel(const VariableConceptHandle &indices, const Dim dim, T buffer);

  [[nodiscard]] VariableConceptHandle clone() const override;

  bool operator==(const BinArrayModel &other) const noexcept;

  bool operator!=(const BinArrayModel &other) const noexcept {
    return !(*this == other);
  }

  [[nodiscard]] VariableConceptHandle
  makeDefaultFromParent(const scipp::index size) const override;

  [[nodiscard]] VariableConceptHandle
  makeDefaultFromParent(const Variable &shape) const override;

  static DType static_dtype() noexcept { return scipp::dtype<bucket<T>>; }
  [[nodiscard]] DType dtype() const noexcept override {
    return scipp::dtype<bucket<T>>;
  }

  [[nodiscard]] bool equals(const Variable &a,
                            const Variable &b) const override;
  void copy(const Variable &src, Variable &dest) const override;
  void copy(const Variable &src, Variable &&dest) const override;
  void assign(const VariableConcept &other) override;

  // TODO Should the mutable version return a view to prevent risk of clients
  // breaking invariants of variable?
  const T &buffer() const noexcept { return m_buffer; }
  T &buffer() noexcept { return m_buffer; }

  ElementArrayView<bucket<T>> values(const core::ElementArrayViewParams &base) {
    return {index_values(base), this->bin_dim(), m_buffer};
  }
  ElementArrayView<const bucket<T>>
  values(const core::ElementArrayViewParams &base) const {
    return {index_values(base), this->bin_dim(), m_buffer};
  }

  [[nodiscard]] scipp::index dtype_size() const override {
    return sizeof(scipp::index_pair);
  }

  void setVariances(const Variable &) override {
    except::throw_cannot_have_variances(core::dtype<core::bin<T>>);
  }

private:
  ElementArrayView<const scipp::index_pair>
  index_values(const core::ElementArrayViewParams &base) const;
  T m_buffer;
};

template <class T> BinArrayModel<T> copy(const BinArrayModel<T> &model);

template <class T>
bool BinArrayModel<T>::equals(const Variable &a, const Variable &b) const {
  // TODO This implementation is slow since it creates a view for every bucket.
  return a.dtype() == dtype() && b.dtype() == dtype() &&
         equals_impl(a.values<bucket<T>>(), b.values<bucket<T>>());
}

template <class T>
void BinArrayModel<T>::copy(const Variable &src, Variable &dest) const {
  const auto &[indices0, dim0, buffer0] = src.constituents<T>();
  auto &&[indices1, dim1, buffer1] = dest.constituents<T>();
  static_cast<void>(dim1);
  copy_slices(buffer0, buffer1, dim0, indices0, indices1);
}

template <class T>
void BinArrayModel<T>::copy(const Variable &src, Variable &&dest) const {
  copy(src, dest);
}

} // namespace scipp::variable
