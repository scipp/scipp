// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/bucket.h"
#include "scipp/core/element_array_view.h"

namespace scipp::core {

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

  /// Access bucket element values. Used internally in transform.
  ///
  /// Note that the returned view is not fully valid, in particular access such
  /// as begin() and end() must not be used.
  template <class Elem> auto values() const noexcept {
    return make_nested(m_transform.m_buffer->template values<Elem>().data());
  }
  /// Access bucket element variances. Used internally in transform.
  ///
  /// Note that the returned view is not fully valid, in particular access such
  /// as begin() and end() must not be used.
  template <class Elem> auto variances() const noexcept {
    return make_nested(m_transform.m_buffer->template variances<Elem>().data());
  }
  bool hasVariances() const noexcept {
    return m_transform.m_buffer->hasVariances();
  }

private:
  struct make_item {
    auto operator()(typename bucket_array_view::value_type &range) const {
      return m_buffer->slice({m_dim, range.first, range.second});
    }
    Dim m_dim;
    T *m_buffer;
  };
  template <class Elem> auto make_nested(Elem *buffer) const {
    return core::ElementArrayView(buffer, m_offset, m_iterDims, m_dataDims,
                                  {m_transform.m_dim,
                                   m_transform.m_buffer->dims(),
                                   scipp::span{data(), data() + size()}});
  }
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
