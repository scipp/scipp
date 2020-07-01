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

void expectCanBroadcastFromTo(const Dimensions &source,
                              const Dimensions &target);

/// A view into multi-dimensional data, supporting slicing, index reordering,
/// and broadcasting.
template <class T> class SCIPP_CORE_EXPORT ElementArrayView {
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;

  /// Construct a ElementArrayView.
  ///
  /// @param variable Pointer to data array.
  /// @param offset Start offset from beginning of data array.
  /// @param targetDimensions Dimensions of the constructed ElementArrayView.
  /// @param dimensions Dimensions of the data array.
  ///
  /// The parameter `targetDimensions` can be used to remove, slice, broadcast,
  /// or transpose dimensions of the input data array.
  ElementArrayView(T *variable, const scipp::index offset,
                   const Dimensions &targetDimensions,
                   const Dimensions &dimensions)
      : m_variable(variable), m_offset(offset),
        m_targetDimensions(targetDimensions), m_dimensions(dimensions) {
    expectCanBroadcastFromTo(m_dimensions, m_targetDimensions);
  }

  /// Construct a ElementArrayView from another ElementArrayView, with different
  /// target dimensions.
  ///
  /// A good way to think of this is of a non-contiguous underlying data array,
  /// e.g., since the other view may represent a slice. This also supports
  /// broadcasting the slice.
  template <class Other>
  ElementArrayView(const Other &other, const Dimensions &targetDimensions)
      : m_variable(other.m_variable), m_offset(other.m_offset),
        m_targetDimensions(targetDimensions) {
    expectCanBroadcastFromTo(other.m_targetDimensions, m_targetDimensions);
    m_dimensions = other.m_dimensions;
    // See implementation of ViewIndex regarding this relabeling.
    for (const auto label : m_dimensions.labels())
      if (label != Dim::Invalid && !other.m_targetDimensions.contains(label))
        m_dimensions.relabel(m_dimensions.index(label), Dim::Invalid);
  }

  /// Construct a ElementArrayView from another ElementArrayView, with different
  /// target dimensions and offset derived from `dim` and `begin`.
  ///
  /// This is essentially performing a slice of a ElementArrayView, creating a
  /// new view which may at the same time also perform other manipulations such
  /// as broadcasting and transposing.
  template <class Other>
  ElementArrayView(const Other &other, const Dimensions &targetDimensions,
                   const Dim dim, const scipp::index begin)
      : m_variable(other.m_variable), m_offset(other.m_offset),
        m_targetDimensions(targetDimensions) {
    expectCanBroadcastFromTo(other.m_targetDimensions, m_targetDimensions);
    m_dimensions = other.m_dimensions;
    if (dim != Dim::Invalid)
      m_offset += begin * m_dimensions.offset(dim);
    // See implementation of ViewIndex regarding this relabeling.
    for (const auto label : m_dimensions.labels())
      if (label != Dim::Invalid && !other.m_targetDimensions.contains(label))
        m_dimensions.relabel(m_dimensions.index(label), Dim::Invalid);
  }

  ElementArrayView<std::remove_const_t<T>>
  createMutable(std::remove_const_t<T> *variable) const {
    return ElementArrayView<std::remove_const_t<T>>(
        variable, m_offset, m_targetDimensions, m_dimensions);
  }

  friend class ElementArrayView<std::remove_const_t<T>>;
  friend class ElementArrayView<const T>;

  class iterator
      : public boost::iterator_facade<iterator, T,
                                      boost::random_access_traversal_tag> {
  public:
    iterator(T *variable, const Dimensions &targetDimensions,
             const Dimensions &dimensions, const scipp::index index)
        : m_variable(variable), m_index(targetDimensions, dimensions) {
      m_index.setIndex(index);
    }

  private:
    friend class boost::iterator_core_access;

    bool equal(const iterator &other) const { return m_index == other.m_index; }
    constexpr void increment() noexcept { m_index.increment(); }
    auto &dereference() const { return m_variable[m_index.get()]; }
    void decrement() { m_index.setIndex(m_index.index() - 1); }
    void advance(int64_t delta) {
      if (delta == 1)
        increment();
      else
        m_index.setIndex(m_index.index() + delta);
    }
    int64_t distance_to(const iterator &other) const {
      return static_cast<int64_t>(other.m_index.index()) -
             static_cast<int64_t>(m_index.index());
    }

    T *m_variable;
    ViewIndex m_index;
  };

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

  ViewIndex begin_index() const noexcept {
    return {m_targetDimensions, m_dimensions};
  }
  ViewIndex end_index() const noexcept {
    ViewIndex i{m_targetDimensions, m_dimensions};
    i.setIndex(size());
    return i;
  }

  scipp::index size() const { return m_targetDimensions.volume(); }

  bool operator==(const ElementArrayView<T> &other) const {
    if (m_targetDimensions != other.m_targetDimensions)
      return false;
    return std::equal(begin(), end(), other.begin());
  }

  /// Return true if *this and other are two equivalent views of the same data.
  bool isSame(const ElementArrayView<T> &other) const {
    return (m_variable == other.m_variable) && (m_offset == other.m_offset) &&
           (m_dimensions == other.m_dimensions) &&
           (m_targetDimensions == other.m_targetDimensions);
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

  const Dimensions &parentDimensions() const { return m_dimensions; }

private:
  T *m_variable;
  scipp::index m_offset{0};
  Dimensions m_targetDimensions;
  Dimensions m_dimensions;
};

template <class T>
ElementArrayView<T> makeElementArrayView(T *variable, const scipp::index offset,
                                         const Dimensions &targetDimensions,
                                         const Dimensions &dimensions) {
  return ElementArrayView<T>(variable, offset, targetDimensions, dimensions);
}

template <class T> struct is_ElementArrayView : std::false_type {};
template <class T>
struct is_ElementArrayView<ElementArrayView<T>> : std::true_type {};
template <class T>
inline constexpr bool is_ElementArrayView_v = is_ElementArrayView<T>::value;

} // namespace scipp::core

namespace scipp {
using core::ElementArrayView;
}
