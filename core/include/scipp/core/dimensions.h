// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "scipp/core/index.h"
#include "scipp/core/span.h"
#include "scipp/units/unit.h"

namespace scipp::core {

constexpr int32_t NDIM_MAX = 6;

/// Dimensions are accessed very frequently, so packing everything into a single
/// (64 Byte) cacheline should be advantageous.
/// We follow the numpy convention: First dimension is outer dimension, last
/// dimension is inner dimension.
class Dimensions {
public:
  static constexpr auto Sparse = std::numeric_limits<scipp::index>::min();

  constexpr Dimensions() noexcept {}
  Dimensions(const Dim dim, const scipp::index size)
      : Dimensions({{dim, size}}) {}
  Dimensions(const std::vector<Dim> &labels,
             const std::vector<scipp::index> &shape);
  Dimensions(const std::initializer_list<std::pair<Dim, scipp::index>> dims) {
    for (const auto[label, size] : dims)
      addInner(label, size);
  }

  constexpr bool operator==(const Dimensions &other) const noexcept {
    if (m_ndim != other.m_ndim)
      return false;
    for (int32_t i = 0; i < NDIM_MAX; ++i) {
      if (m_shape[i] != other.m_shape[i])
        return false;
      if (m_dims[i] != other.m_dims[i])
        return false;
    }
    if (m_dims[NDIM_MAX] != other.m_dims[NDIM_MAX])
      return false;
    return true;
  }
  constexpr bool operator!=(const Dimensions &other) const noexcept {
    return !(*this == other);
  }

  constexpr bool empty() const noexcept { return m_ndim == 0 && !sparse(); }

  /// Return the volume of the space defined by *this. If there is a sparse
  /// dimension the volume of the dense subspace is returned.
  constexpr scipp::index volume() const noexcept {
    scipp::index volume{1};
    for (int32_t dim = 0; dim < m_ndim; ++dim)
      volume *= m_shape[dim];
    return volume;
  }

  /// Return true if there is a sparse dimension.
  constexpr bool sparse() const noexcept {
    return m_dims[m_ndim] != Dim::Invalid;
  }

  /// Return the label of a potential sparse dimension, Dim::Invalid otherwise.
  constexpr Dim sparseDim() const noexcept { return m_dims[m_ndim]; }

  scipp::span<const scipp::index> shape() const && = delete;
  /// Return the shape of the space defined by *this. If there is a sparse
  /// dimension the shape of the dense subspace is returned.
  constexpr scipp::span<const scipp::index> shape() const &noexcept {
    return {m_shape, m_shape + m_ndim};
  }

  scipp::span<const Dim> labels() const && = delete;
  /// Return the labels of the space defined by *this, including the label of a
  /// potential sparse dimension.
  constexpr scipp::span<const Dim> labels() const &noexcept {
    if (!sparse())
      return {m_dims, m_dims + m_ndim};
    else
      return {m_dims, m_dims + m_ndim + 1};
  }

  scipp::span<const Dim> denseLabels() const && = delete;
  /// Return the labels of the space defined by *this, excluding the label of a
  /// potential sparse dimension.
  constexpr scipp::span<const Dim> denseLabels() const &noexcept {
    return {m_dims, m_dims + m_ndim};
  }

  scipp::index operator[](const Dim dim) const;

  Dim inner() const;

  /// Return true if `dim` is one of the labels in *this.
  constexpr bool contains(const Dim dim) const noexcept {
    return denseContains(dim) || (sparse() && sparseDim() == dim);
  }

  /// Return true if `dim` is one of the dense labels in *this.
  constexpr bool denseContains(const Dim dim) const noexcept {
    for (int32_t i = 0; i < m_ndim; ++i)
      if (m_dims[i] == dim)
        return true;
    return false;
  }

  bool contains(const Dimensions &other) const noexcept;

  bool isContiguousIn(const Dimensions &parent) const;

  // TODO Some of the following methods are probably legacy and should be
  // considered for removal.
  Dim label(const scipp::index i) const;
  void relabel(const scipp::index i, const Dim label) { m_dims[i] = label; }
  scipp::index size(const scipp::index i) const;
  scipp::index offset(const Dim label) const;
  void resize(const Dim label, const scipp::index size);
  void resize(const scipp::index i, const scipp::index size);
  void erase(const Dim label);

  // TODO Better names required.
  void add(const Dim label, const scipp::index size);
  void addInner(const Dim label, const scipp::index size);

  int32_t index(const Dim label) const;

private:
  scipp::index &at(const Dim dim);
  // This is 56 Byte, or would be 40 Byte for small size 1.
  // boost::container::small_vector<std::pair<Dim, scipp::index>, 2> m_dims;
  // Support at most 6 dimensions, should be sufficient?
  // 6*8 Byte = 48 Byte
  // TODO: can we find a good way to initialise the values automatically when
  // NDIM_MAX is changed?
  scipp::index m_shape[NDIM_MAX]{-1, -1, -1, -1, -1, -1};
  int16_t m_ndim{0};
  /// Dimensions labels. This is exceeding the size of the shape by one for the
  /// purpose of storing the sparse dimension's label.
  Dim m_dims[NDIM_MAX + 1]{Dim::Invalid, Dim::Invalid, Dim::Invalid,
                           Dim::Invalid, Dim::Invalid, Dim::Invalid,
                           Dim::Invalid};
};

Dimensions merge(const Dimensions &a, const Dimensions &b);

} // namespace scipp::core

#endif // DIMENSIONS_H
