// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>

#include <boost/iterator/iterator_facade.hpp>

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
  element_array_view(const scipp::index offset,
                     const Dimensions &targetDimensions,
                     const Dimensions &dimensions);
  element_array_view(const element_array_view &other,
                     const Dimensions &targetDimensions);

  ViewIndex begin_index() const noexcept {
    return {m_targetDimensions, m_dimensions};
  }
  ViewIndex end_index() const noexcept {
    ViewIndex i{m_targetDimensions, m_dimensions};
    i.setIndex(size());
    return i;
  }

  scipp::index size() const { return m_targetDimensions.volume(); }

protected:
  scipp::index m_offset{0};
  Dimensions m_targetDimensions;
  Dimensions m_dimensions;
};

/// A view into multi-dimensional data, supporting slicing, index reordering,
/// and broadcasting.
template <class T>
class SCIPP_CORE_EXPORT ElementArrayView : public element_array_view {
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using iterator = element_array_view_iterator<T>;

  /// Construct an ElementArrayView over given data pointer.
  ElementArrayView(T *variable, const scipp::index offset,
                   const Dimensions &targetDimensions,
                   const Dimensions &dimensions)
      : element_array_view(offset, targetDimensions, dimensions),
        m_variable(variable) {}

  /// Construct a ElementArrayView from another ElementArrayView, with different
  /// target dimensions.
  template <class Other>
  ElementArrayView(const Other &other, const Dimensions &targetDimensions)
      : element_array_view(other, targetDimensions),
        m_variable(other.m_variable) {}

  friend class ElementArrayView<std::remove_const_t<T>>;
  friend class ElementArrayView<const T>;

  iterator begin() const {
    return {m_variable + m_offset, m_targetDimensions, m_dimensions, 0};
  }
  iterator end() const {
    return {m_variable + m_offset, m_targetDimensions, m_dimensions, size()};
  }
  auto &operator[](const scipp::index i) const { return *(begin() + i); }

  auto &front() const { return *begin(); }
  auto &back() const { return *(begin() + (size() - 1)); }

  const T *data() const { return m_variable + m_offset; }
  T *data() { return m_variable + m_offset; }

  bool operator==(const ElementArrayView<T> &other) const {
    if (m_targetDimensions != other.m_targetDimensions)
      return false;
    return std::equal(begin(), end(), other.begin());
  }

  template <class T2> bool overlaps(const ElementArrayView<T2> &other) const {
    // TODO We could be less restrictive here and use a more sophisticated check
    // based on offsets and dimensions, if there is a performance issue due to
    // this current stricter requirement.
    if (m_variable == other.m_variable)
      return (m_offset != other.m_offset) ||
             (m_dimensions != other.m_dimensions);
    return false;
  }

private:
  T *m_variable;
};

template <class T> struct is_ElementArrayView : std::false_type {};
template <class T>
struct is_ElementArrayView<ElementArrayView<T>> : std::true_type {};
template <class T>
inline constexpr bool is_ElementArrayView_v = is_ElementArrayView<T>::value;

} // namespace scipp::core

namespace scipp {
using core::ElementArrayView;
}
