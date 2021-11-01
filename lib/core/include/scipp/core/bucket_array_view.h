// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/bucket.h"
#include "scipp/core/element_array_view.h"

namespace scipp::core {

template <class T>
class bucket_array_view : public ElementArrayView<const scipp::index_pair> {
public:
  using value_type = const scipp::index_pair;
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
      return ElementArrayViewParams::overlaps(other);
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
  using value_type = T;
  using bucket_array_view<T>::bucket_array_view;
};

/// Specialization of ElementArrayView for const access to bucketed data.
///
/// Iteration returns a const view to a slice of the underlying buffer. For
/// example, a VariableConstView in case of T=Variable.
template <class T>
class ElementArrayView<const bucket<T>> : public bucket_array_view<const T> {
public:
  using value_type = T;
  using bucket_array_view<const T>::bucket_array_view;
};

} // namespace scipp::core
