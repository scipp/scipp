/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef VARIABLE_VIEW_H
#define VARIABLE_VIEW_H

#include <boost/iterator/iterator_facade.hpp>

#include "dimensions.h"
#include "multi_index.h"

template <class T> class VariableView {
public:
  VariableView(T &variable, const Dimensions &targetDimensions,
               const Dimensions &dimensions)
      : m_variable(variable), m_targetDimensions(targetDimensions),
        m_dimensions(dimensions) {}

  class iterator
      : public boost::iterator_facade<
            iterator, std::conditional_t<std::is_const<T>::value,
                                         const typename T::value_type,
                                         typename T::value_type>,
            boost::random_access_traversal_tag> {
  public:
    iterator(T &variable, const Dimensions &targetDimensions,
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

    T &m_variable;
    MultiIndex m_index;
  };

  iterator begin() const {
    return {m_variable, m_targetDimensions, m_dimensions, 0};
  }
  iterator end() const {
    return {m_variable, m_targetDimensions, m_dimensions,
            m_targetDimensions.volume()};
  }

private:
  T &m_variable;
  const Dimensions m_targetDimensions;
  const Dimensions m_dimensions;
};

#endif // VARIABLE_VIEW_H
