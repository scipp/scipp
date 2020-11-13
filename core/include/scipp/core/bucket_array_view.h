// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/bucket.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/typed_bin.h"

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

template <class T>
class typed_bin_array_view
    : public ElementArrayView<const bucket_base::range_type> {
public:
  using value_type = const typename bucket_base::range_type;
  using base = ElementArrayView<const value_type>;

  typed_bin_array_view(const base &buckets, const Dim ignored_dim,
                       const typed_bin<T> &bin)
      : base(buckets), m_bin{bin} {
    static_cast<void>(ignored_dim);
  }

  template <class Other>
  typed_bin_array_view(const Other &other, const Dimensions &iterDims)
      : base(other, iterDims), m_bin(other.bin()) {}
  // typed_bin_array_view(const base &buckets, const scipp::span<T> &values,
  //                     const scipp::span<T> &variances)
  //    : base(buckets), m_transform{values, variances} {}
  // if (buffer.dims().ndim() != 1)
  //  throw std::runtime_error(
  //      "Bins with extra dimensions cannot be accessed as spans.");

  auto begin() const {
    return boost::make_transform_iterator(base::begin(), m_bin);
  }
  auto end() const {
    return boost::make_transform_iterator(base::end(), m_bin);
  }

  auto operator[](const scipp::index i) const { return *(begin() + i); }

  auto front() const { return *begin(); }
  auto back() const { return *(begin() + (size() - 1)); }

  bool operator==(const typed_bin_array_view &other) const {
    // TODO
    return false;
    // if (dims() != other.dims())
    //  return false;
    // return m_bin == other.m_bin;
  }

  template <class T2>
  bool overlaps(const typed_bin_array_view<T2> &other) const {
    // TODO
    return false;
  }

  auto bin() const noexcept { return m_bin; };

private:
  typed_bin<T> m_bin;
};

template <class T>
class ElementArrayView<bucket<typed_bin<T>>> : public typed_bin_array_view<T> {
public:
  using value_type = typed_bin<T>;
  using typed_bin_array_view<T>::typed_bin_array_view;
};

template <class T>
class ElementArrayView<const bucket<typed_bin<T>>>
    : public typed_bin_array_view<T> {
public:
  using value_type = typed_bin<T>;
  using typed_bin_array_view<T>::typed_bin_array_view;
};

} // namespace scipp::core
