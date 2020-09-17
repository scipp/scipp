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

struct SCIPP_CORE_EXPORT BucketParams {
  bool operator==(const BucketParams &other) const noexcept {
    // TODO actually we need to check equality based on data/iter dims to handle
    // slicing
    // TODO more severa problem: if indices differ, we also need to load
    // different bucket offsets, even if sizes are them same!
    return dim == other.dim && dims == other.dims && indices == other.indices;
    // std::equal(indices.begin(), indices.end(), other.indices.begin(),
    //           other.indices.end());
  }
  bool operator!=(const BucketParams &other) const noexcept {
    return !(*this == other);
  }
  explicit operator bool() const noexcept { return *this != BucketParams{}; }
  Dim dim{Dim::Invalid};
  Dimensions dims{};
  const std::pair<scipp::index, scipp::index> *indices{nullptr};
};

constexpr auto merge(const BucketParams &a) noexcept { return a; }
inline auto merge(const BucketParams &a, const BucketParams &b) {
  if (a != b)
    throw std::runtime_error("Mismatching bucket sizes");
  return a ? a : b;
}

template <class... Ts>
auto merge(const BucketParams &a, const BucketParams &b, const Ts &... params) {
  return merge(merge(a, b), params...);
}

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
                     const Dimensions &dataDims,
                     const BucketParams &bucketParams);
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
  constexpr const Dimensions &dataDims() const noexcept { return m_dataDims; }
  constexpr const BucketParams &bucketParams() const noexcept {
    return m_bucketParams;
  }

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
  BucketParams m_bucketParams{};
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
                   const Dimensions &iterDims, const Dimensions &dataDims,
                   const BucketParams &bucketParams = BucketParams{})
      : element_array_view(offset, iterDims, dataDims, bucketParams),
        m_buffer(buffer) {}

  /// Construct an ElementArrayView over given buffer.
  ElementArrayView(const element_array_view &base, T *buffer)
      : element_array_view(base), m_buffer(buffer) {}

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

} // namespace scipp::core

namespace scipp {
using core::ElementArrayView;
}

// Specializations of ElementArrayView:
#include "scipp/core/bucket_array_view.h"
