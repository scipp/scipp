/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#include <memory>
#include <utility>
#include <vector>

#include "dimension.h"
#include "except.h"
#include "index.h"
#include "span.h"

namespace scipp::core {

/// Dimensions are accessed very frequently, so packing everything into a single
/// (64 Byte) cacheline should be advantageous.
/// We should follow the numpy convention: First dimension is outer dimension,
/// last dimension is inner dimension, for now we do not.
class Dimensions {
public:
  Dimensions() noexcept {}
  Dimensions(const Dim dim, const scipp::index size)
      : Dimensions({{dim, size}}) {}
  Dimensions(const std::vector<Dim> &labels,
             const std::vector<scipp::index> &shape) {
    if (labels.size() != shape.size())
      throw std::runtime_error("Constructing Dimensions: Number of dimensions "
                               "labels does not match shape.");
    for (scipp::index i = labels.size() - 1; i >= 0; --i)
      add(labels[i], shape[i]);
  }
  Dimensions(const std::initializer_list<std::pair<Dim, scipp::index>> dims) {
    // TODO Check for duplicate dimension.
    if (dims.size() > 6)
      throw std::runtime_error("At most 6 dimensions are supported.");
    m_ndim = static_cast<int32_t>(dims.size());
    auto dim = dims.begin();
    for (scipp::index i = 0; i < m_ndim; ++i, ++dim) {
      m_dims[i] = dim->first;
      if (m_dims[i] == Dim::Invalid)
        throw std::runtime_error("Dim::Invalid is not a valid dimension.");
      m_shape[i] = dim->second;
      if (m_shape[i] < 0)
        throw std::runtime_error("Dimension extent cannot be negative.");
    }
  }

  bool operator==(const Dimensions &other) const noexcept {
    if (m_ndim != other.m_ndim)
      return false;
    for (int32_t i = 0; i < 6; ++i) {
      if (m_shape[i] != other.m_shape[i])
        return false;
      if (m_dims[i] != other.m_dims[i])
        return false;
    }
    return true;
  }
  bool operator!=(const Dimensions &other) const noexcept {
    return !(*this == other);
  }

  bool empty() const noexcept { return m_ndim == 0; }

  int32_t ndim() const noexcept { return m_ndim; }
  // TODO Remove in favor of the new ndim?
  int32_t count() const noexcept { return m_ndim; }

  scipp::index volume() const noexcept {
    scipp::index volume{1};
    for (int32_t dim = 0; dim < ndim(); ++dim)
      volume *= m_shape[dim];
    return volume;
  }

  scipp::span<const scipp::index> shape() const noexcept {
    return {m_shape, m_shape + m_ndim};
  }

  scipp::span<const Dim> labels() const noexcept {
    return {m_dims, m_dims + m_ndim};
  }

  scipp::index operator[](const Dim dim) const {
    for (int32_t i = 0; i < 6; ++i)
      if (m_dims[i] == dim)
        return m_shape[i];
    throw except::DimensionNotFoundError(*this, dim);
  }

  scipp::index &operator[](const Dim dim) {
    for (int32_t i = 0; i < 6; ++i)
      if (m_dims[i] == dim)
        return m_shape[i];
    throw except::DimensionNotFoundError(*this, dim);
  }

  bool contains(const Dim dim) const noexcept {
    for (int32_t i = 0; i < ndim(); ++i)
      if (m_dims[i] == dim)
        return true;
    return false;
  }

  bool contains(const Dimensions &other) const;

  bool isContiguousIn(const Dimensions &parent) const;

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

  Dim inner() const;

  int32_t index(const Dim label) const;

private:
  // This is 56 Byte, or would be 40 Byte for small size 1.
  // boost::container::small_vector<std::pair<Dim, scipp::index>, 2> m_dims;
  // Support at most 6 dimensions, should be sufficient?
  // 6*8 Byte = 48 Byte
  scipp::index m_shape[6]{-1, -1, -1, -1, -1, -1};
  int32_t m_ndim{0};
  Dim m_dims[6]{Dim::Invalid, Dim::Invalid, Dim::Invalid,
                Dim::Invalid, Dim::Invalid, Dim::Invalid};
};

} // namespace scipp::core

#endif // DIMENSIONS_H
