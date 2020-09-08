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

  // TODO data() returns base::data... can we even use this in transform like
  // this?
  template <class Elem> auto values() const noexcept {
    // should convert this into element_array_view with begin_index() that
    // returns a ViewIndex with subspan setup?
    // maybe just a minimal helper class, providing everything transform needs,
    // i.e.,
    // not begin_index! transform uses
    // data()
    // dims()
    // dataDims()
    // => need to add indices, nested dim, buffer dims... from multiple inputs,
    // and validate helper need to set BucketParams and use iterDims and
    // dataDims from this, not m_buffer
    // ... but data() needs to return events, not bucket indices (ok as is?)
    // TODO note that all other accessors will be broken... how to manage access
    // control?
    auto view = m_transform.m_buffer->template values<Elem>();
    view.m_offset = m_offset;
    view.m_iterDims = m_iterDims;
    view.m_dataDims = m_dataDims;
    // should MultiIndex handle iter of bucket array, or should we pass full
    // begin/end iterators here (or just the whole view)?
    view.m_bucketParams =
        BucketParams{m_transform.m_dim, m_transform.m_buffer->dims(),
                     scipp::span{data(), data() + size()}};
    return view;
  }
  template <class Elem> auto variances() const noexcept {
    auto view = m_transform.m_buffer->template variances<Elem>();
    view.m_offset = m_offset;
    view.m_iterDims = m_iterDims;
    view.m_dataDims = m_dataDims;
    view.m_bucketParams =
        BucketParams{m_transform.m_dim, m_transform.m_buffer->dims(),
                     scipp::span{data(), data() + size()}};
    return view;
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
