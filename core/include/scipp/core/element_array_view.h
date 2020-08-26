// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/bucket.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/view_index.h"

namespace scipp::core {

template <class T>
class element_array_view_iterator
    : public boost::iterator_facade<element_array_view_iterator<T>, T,
                                    boost::random_access_traversal_tag> {
public:
  element_array_view_iterator(T *variable, const Dimensions &targetDimensions,
                              const Dimensions &dimensions,
                              const scipp::index index)
      : m_variable(variable), m_index(targetDimensions, dimensions) {
    m_index.setIndex(index);
  }

private:
  friend class boost::iterator_core_access;

  bool equal(const element_array_view_iterator &other) const {
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
  int64_t distance_to(const element_array_view_iterator &other) const {
    return static_cast<int64_t>(other.m_index.index()) -
           static_cast<int64_t>(m_index.index());
  }

  T *m_variable;
  ViewIndex m_index;
};

/// Base class for ElementArrayView<T> with functionality independent of T.
class SCIPP_CORE_EXPORT element_array_view {
public:
  element_array_view(const scipp::index offset, const Dimensions &iterDims,
                     const Dimensions &dataDims);
  element_array_view(const element_array_view &other,
                     const Dimensions &iterDims);

  ViewIndex begin_index() const noexcept { return {m_iterDims, m_dataDims}; }
  ViewIndex end_index() const noexcept {
    ViewIndex i{m_iterDims, m_dataDims};
    i.setIndex(size());
    return i;
  }

  scipp::index size() const { return m_iterDims.volume(); }
  constexpr const Dimensions &dims() const noexcept { return m_iterDims; }

  bool overlaps(const element_array_view &other) const {
    // TODO We could be less restrictive here and use a more sophisticated check
    // based on offsets and dimensions, if there is a performance issue due to
    // this current stricter requirement.
    return (m_offset != other.m_offset) || (m_dataDims != other.m_dataDims);
  }

protected:
  scipp::index m_offset{0};
  Dimensions m_iterDims;
  Dimensions m_dataDims;
};

/// A view into multi-dimensional data, supporting slicing, index reordering,
/// and broadcasting.
template <class T>
class SCIPP_CORE_EXPORT ElementArrayView : public element_array_view {
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using iterator = element_array_view_iterator<T>;

  /// Construct an ElementArrayView over given buffer.
  ElementArrayView(T *buffer, const scipp::index offset,
                   const Dimensions &iterDims, const Dimensions &dataDims)
      : element_array_view(offset, iterDims, dataDims), m_buffer(buffer) {}

  /// Construct a ElementArrayView from another ElementArrayView, with different
  /// iteration dimensions.
  template <class Other>
  ElementArrayView(const Other &other, const Dimensions &iterDims)
      : element_array_view(other, iterDims), m_buffer(other.buffer()) {}

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

  bool operator==(const ElementArrayView<T> &other) const {
    if (dims() != other.dims())
      return false;
    return std::equal(begin(), end(), other.begin());
  }

  template <class T2> bool overlaps(const ElementArrayView<T2> &other) const {
    if (buffer() == other.buffer())
      return element_array_view::overlaps(other);
    return false;
  }

  constexpr T *buffer() const noexcept { return m_buffer; }

private:
  T *m_buffer;
};

template <class T>
class bucket_array_view
    : public ElementArrayView<const bucket_base::range_type> {
public:
  using value_type = const typename bucket_base::range_type;
  using base = ElementArrayView<const value_type>;

  bucket_array_view(const base &buckets, const Dim dim, T &buffer)
      : base(buckets), m_transform{dim, &buffer} {}

  auto begin() const {
    return boost::make_transform_iterator(base::begin(), m_transform);
  }
  auto end() const {
    return boost::make_transform_iterator(base::end(), m_transform);
  }

  auto operator[](const scipp::index i) const { return *(begin() + i); }

  auto front() const { return *begin(); }
  auto back() const { return *(begin() + (size() - 1)); }

  bool operator==(const bucket_array_view &other) const {
    if (dims() != other.dims())
      return false;
    return std::equal(begin(), end(), other.begin());
  }

  template <class T2> bool overlaps(const bucket_array_view<T2> &other) const {
    if (buffer() == other.buffer())
      return element_array_view::overlaps(other);
    return false;
  }

private:
  struct make_item {
    auto operator()(typename bucket_array_view::value_type &range) const {
      return m_buffer->slice({m_dim, range.first, range.second});
    }
    Dim m_dim;
    T *m_buffer;
  };
  make_item m_transform;
};

/// Specialization of ElementArrayView for mutable access to bucketed data.
///
/// Iteration returns a mutable view to a slice of the underlying buffer. For
/// example, a VariableView in case of T=Variable.
template <class T>
class ElementArrayView<bucket<T>> : public bucket_array_view<T> {
public:
  using value_type = typename T::view_type;
  using bucket_array_view<T>::bucket_array_view;
};

/// Specialization of ElementArrayView for const access to bucketed data.
///
/// Iteration returns a const view to a slice of the underlying buffer. For
/// example, a VariableConstView in case of T=Variable.
template <class T>
class ElementArrayView<const bucket<T>> : public bucket_array_view<const T> {
public:
  using value_type = typename T::const_view_type;
  using bucket_array_view<const T>::bucket_array_view;
};

} // namespace scipp::core

namespace scipp {
using core::ElementArrayView;
}
