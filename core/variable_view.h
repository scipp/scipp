// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef VARIABLE_VIEW_H
#define VARIABLE_VIEW_H

#include <algorithm>

#include <boost/iterator/iterator_facade.hpp>

#include "dimensions.h"
#include "vector.h"
#include "view_index.h"

namespace scipp::core {

/// A view into multi-dimensional data, supporting slicing, index reordering,
/// and broadcasting.
template <class T> class VariableView {
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;

  VariableView(T *variable, const scipp::index offset,
               const Dimensions &targetDimensions, const Dimensions &dimensions)
      : m_variable(variable), m_offset(offset),
        m_targetDimensions(targetDimensions), m_dimensions(dimensions) {}

  template <class Other>
  VariableView(const Other &other, const Dimensions &targetDimensions)
      : m_variable(other.m_variable), m_offset(other.m_offset),
        m_targetDimensions(targetDimensions) {
    m_dimensions = other.m_dimensions;
    for (const auto label : m_dimensions.labels())
      if (!other.m_targetDimensions.denseContains(label))
        m_dimensions.relabel(m_dimensions.index(label), Dim::Invalid);
  }

  template <class Other>
  VariableView(const Other &other, const Dimensions &targetDimensions,
               const Dim dim, const scipp::index begin)
      : m_variable(other.m_variable), m_offset(other.m_offset),
        m_targetDimensions(targetDimensions) {
    m_dimensions = other.m_dimensions;
    if (begin != 0 || dim != Dim::Invalid)
      m_offset += begin * m_dimensions.offset(dim);
    for (const auto label : m_dimensions.labels())
      if (!other.m_targetDimensions.denseContains(label))
        m_dimensions.relabel(m_dimensions.index(label), Dim::Invalid);
  }

  VariableView<std::remove_const_t<T>>
  createMutable(std::remove_const_t<T> *variable) const {
    return VariableView<std::remove_const_t<T>>(
        variable, m_offset, m_targetDimensions, m_dimensions);
  }

  friend class VariableView<std::remove_const_t<T>>;
  friend class VariableView<const T>;

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
    void increment() { m_index.increment(); }
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

  const T *data() const { return m_variable + m_offset; }
  T *data() { return m_variable + m_offset; }

  scipp::index size() const { return m_targetDimensions.volume(); }

  bool operator==(const VariableView<T> &other) const {
    if (m_targetDimensions != other.m_targetDimensions)
      return false;
    return std::equal(begin(), end(), other.begin());
  }

  bool overlaps(const VariableView<const T> &other) const {
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
VariableView<T> makeVariableView(T *variable, const scipp::index offset,
                                 const Dimensions &targetDimensions,
                                 const Dimensions &dimensions) {
  return VariableView<T>(variable, offset, targetDimensions, dimensions);
}

} // namespace scipp::core

#endif // VARIABLE_VIEW_H
