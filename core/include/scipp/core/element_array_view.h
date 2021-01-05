// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>

#include <boost/iterator/iterator_facade.hpp>

#include "scipp/core/dimensions.h"
#include "scipp/core/view_index.h"

namespace scipp::core {

struct SCIPP_CORE_EXPORT BucketParams {
  explicit operator bool() const noexcept { return dim != Dim::Invalid; }
  Dim dim{Dim::Invalid};
  Dimensions dims{};
  const std::pair<scipp::index, scipp::index> *indices{nullptr};
};

template <class T>
class ElementArrayViewParams_iterator
    : public boost::iterator_facade<ElementArrayViewParams_iterator<T>, T,
                                    boost::random_access_traversal_tag> {
public:
  ElementArrayViewParams_iterator(T *variable,
                                  const Dimensions &targetDimensions,
                                  const Dimensions &dimensions,
                                  const scipp::index index)
      : m_variable(variable), m_index(targetDimensions, dimensions) {
    m_index.setIndex(index);
  }

private:
  friend class boost::iterator_core_access;

  bool equal(const ElementArrayViewParams_iterator &other) const {
    return m_index == other.m_index;
  }
  constexpr void increment() noexcept { m_index.increment(); }
  auto &dereference() const { return m_variable[m_index.get()]; }
  void decrement() { m_index.setIndex(m_index.index() - 1); }
  void advance(int64_t delta) {
    if (delta == 1)
      increment();
    else
      m_index.setIndex(m_index.index() + delta);
  }
  int64_t distance_to(const ElementArrayViewParams_iterator &other) const {
    return static_cast<int64_t>(other.m_index.index()) -
           static_cast<int64_t>(m_index.index());
  }

  T *m_variable;
  ViewIndex m_index;
};

/// Base class for ElementArrayView<T> with functionality independent of T.
class SCIPP_CORE_EXPORT ElementArrayViewParams {
public:
  ElementArrayViewParams(const scipp::index offset, const Dimensions &iterDims,
                         const Dimensions &dataDims,
                         const BucketParams &bucketParams);
  ElementArrayViewParams(const ElementArrayViewParams &other,
                         const Dimensions &iterDims);

  ViewIndex begin_index() const noexcept { return {m_iterDims, m_dataDims}; }
  ViewIndex end_index() const noexcept {
    ViewIndex i{m_iterDims, m_dataDims};
    i.setIndex(size());
    return i;
  }

  scipp::index size() const { return m_iterDims.volume(); }
  constexpr scipp::index offset() const noexcept { return m_offset; }
  constexpr const Dimensions &dims() const noexcept { return m_iterDims; }
  constexpr const Dimensions &dataDims() const noexcept { return m_dataDims; }
  constexpr const BucketParams &bucketParams() const noexcept {
    return m_bucketParams;
  }

  bool overlaps(const ElementArrayViewParams &other) const {
    // TODO We could be less restrictive here and use a more sophisticated check
    // based on offsets and dimensions, if there is a performance issue due to
    // this current stricter requirement.
    return (m_offset != other.m_offset) || (m_dataDims != other.m_dataDims);
  }

protected:
  void requireContiguous() const;
  scipp::index m_offset{0};
  Dimensions m_iterDims;
  Dimensions m_dataDims;
  BucketParams m_bucketParams{};
};

/// A view into multi-dimensional data, supporting slicing, index reordering,
/// and broadcasting.
template <class T>
class SCIPP_CORE_EXPORT ElementArrayView : public ElementArrayViewParams {
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using iterator = ElementArrayViewParams_iterator<T>;

  /// Construct an ElementArrayView over given buffer.
  ElementArrayView(T *buffer, const scipp::index offset,
                   const Dimensions &iterDims, const Dimensions &dataDims,
                   const BucketParams &bucketParams = BucketParams{})
      : ElementArrayViewParams(offset, iterDims, dataDims, bucketParams),
        m_buffer(buffer) {}

  /// Construct an ElementArrayView over given buffer.
  ElementArrayView(const ElementArrayViewParams &base, T *buffer)
      : ElementArrayViewParams(base), m_buffer(buffer) {}

  /// Construct a ElementArrayView from another ElementArrayView, with different
  /// iteration dimensions.
  template <class Other>
  ElementArrayView(const Other &other, const Dimensions &iterDims)
      : ElementArrayViewParams(other, iterDims), m_buffer(other.buffer()) {}

  iterator begin() const {
    return {m_buffer + m_offset, m_iterDims, m_dataDims, 0};
  }
  iterator end() const {
    return {m_buffer + m_offset, m_iterDims, m_dataDims, size()};
  }
  auto &operator[](const scipp::index i) const { return *(begin() + i); }

  auto &front() const { return *begin(); }
  auto &back() const { return *(begin() + (size() - 1)); }

  const T *data() const { return m_buffer + m_offset; }
  T *data() { return m_buffer + m_offset; }

  auto as_span() const {
    requireContiguous();
    return scipp::span(data(), data() + size());
  }

  auto as_span() {
    requireContiguous();
    return scipp::span(data(), data() + size());
  }

  bool operator==(const ElementArrayView<T> &other) const {
    if (dims() != other.dims())
      return false;
    return std::equal(begin(), end(), other.begin());
  }

  template <class T2> bool overlaps(const ElementArrayView<T2> &other) const {
    if (buffer() == other.buffer())
      return ElementArrayViewParams::overlaps(other);
    return false;
  }

  constexpr T *buffer() const noexcept { return m_buffer; }

private:
  T *m_buffer;
};

} // namespace scipp::core

namespace scipp {
using core::ElementArrayView;
}

// Specializations of ElementArrayView:
#include "scipp/core/bucket_array_view.h"
