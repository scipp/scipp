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
#include "vector.h"

// Some type traits to get pointer to underlying data for various possible
// inputs (vector, span, pointer).
template <class T> struct ValueType { using type = typename T::value_type; };
template <class T> struct ValueType<const Vector<T>> { using type = const T; };
template <class T> struct ValueType<gsl::span<T>> { using type = T; };
template <class T> struct ValueType<const gsl::span<T>> { using type = T; };
template <class T> struct ValueType<T *> { using type = T; };
template <class T> struct ValueType<const T *> { using type = const T; };
template <class T> struct ValueType<const T *const> { using type = const T; };

template <class T> struct GetPtr {
  static auto get(T &variable) { return variable.data(); }
};

template <class T> struct GetPtr<T *> {
  static auto get(T *variable) { return variable; }
};

template <class T> struct GetPtr<T *const> {
  static auto get(T *const variable) { return variable; }
};

template <class T> class VariableView {
public:
  VariableView(T &variable, const Dimensions &targetDimensions,
               const Dimensions &dimensions)
      : m_variable(GetPtr<T>::get(variable)),
        m_targetDimensions(targetDimensions), m_dimensions(dimensions) {}

  class iterator : public boost::iterator_facade<
                       iterator,
                       std::conditional_t<std::is_const<T>::value,
                                          const typename ValueType<T>::type,
                                          typename ValueType<T>::type>,
                       boost::random_access_traversal_tag> {
  public:
    iterator(typename ValueType<T>::type *variable,
             const Dimensions &targetDimensions, const Dimensions &dimensions,
             const gsl::index index)
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

    typename ValueType<T>::type *m_variable;
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
  typename ValueType<T>::type *m_variable;
  const Dimensions m_targetDimensions;
  const Dimensions m_dimensions;
};

template <class T>
auto makeVariableView(T &variable, const Dimensions &targetDimensions,
                      const Dimensions &dimensions) {
  return VariableView<T>(variable, targetDimensions, dimensions);
}

#endif // VARIABLE_VIEW_H
