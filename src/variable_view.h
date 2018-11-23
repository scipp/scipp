/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef VARIABLE_VIEW_H
#define VARIABLE_VIEW_H

#include <algorithm>

#include <boost/iterator/iterator_facade.hpp>

#include "dimensions.h"
#include "multi_index.h"
#include "vector.h"

template <class T> class VariableView {
public:
  using value_type = T;

  VariableView(T *variable, const Dimensions &targetDimensions,
               const Dimensions &dimensions)
      : m_variable(variable), m_targetDimensions(targetDimensions),
        m_dimensions(dimensions) {}

  template <class Other>
  VariableView(const Other &other, const Dimensions &targetDimensions)
      : m_variable(other.m_variable), m_targetDimensions(targetDimensions) {
    m_dimensions = other.m_dimensions;
    for (const auto label : m_dimensions.labels())
      if (!other.m_targetDimensions.contains(label))
        m_dimensions.relabel(m_dimensions.index(label), Dim::Invalid);
  }

  template <class Other>
  VariableView(const Other &other, const Dimensions &targetDimensions,
               const Dim dim, const gsl::index begin)
      : m_variable(other.m_variable), m_targetDimensions(targetDimensions) {
    m_dimensions = other.m_dimensions;
    if (begin != 0 || dim != Dim::Invalid)
      m_variable += begin * m_dimensions.offset(dim);
    for (const auto label : m_dimensions.labels())
      if (!other.m_targetDimensions.contains(label))
        m_dimensions.relabel(m_dimensions.index(label), Dim::Invalid);
  }

  friend class VariableView<std::remove_const_t<T>>;
  friend class VariableView<const T>;

  class iterator
      : public boost::iterator_facade<iterator, T,
                                      boost::random_access_traversal_tag> {
  public:
    iterator(T *variable, const Dimensions &targetDimensions,
             const Dimensions &dimensions, const gsl::index index)
        : m_variable(variable), m_index(targetDimensions, {dimensions}) {
      m_index.setIndex(index);
    }

  private:
    friend class boost::iterator_core_access;

    bool equal(const iterator &other) const { return m_index == other.m_index; }
    void increment() { m_index.increment(); }
    auto &dereference() const { return m_variable[m_index.get<0>()]; }
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
    MultiIndex m_index;
  };

  iterator begin() const {
    return {m_variable, m_targetDimensions, m_dimensions, 0};
  }
  iterator end() const {
    return {m_variable, m_targetDimensions, m_dimensions, size()};
  }
  auto &operator[](const gsl::index i) const { return *(begin() + i); }

  const T *data() const { return m_variable; }
  T *data() { return m_variable; }

  gsl::index size() const { return m_targetDimensions.volume(); }

  bool operator==(const VariableView<T> &other) const {
    if (m_targetDimensions != other.m_targetDimensions)
      return false;
    return std::equal(begin(), end(), other.begin());
  }

  const Dimensions &parentDimensions() const { return m_dimensions; }

private:
  T *m_variable;
  const Dimensions m_targetDimensions;
  Dimensions m_dimensions;
};

template <class T>
VariableView<T> makeVariableView(T *variable,
                                 const Dimensions &targetDimensions,
                                 const Dimensions &dimensions) {
  return VariableView<T>(variable, targetDimensions, dimensions);
}

#endif // VARIABLE_VIEW_H
