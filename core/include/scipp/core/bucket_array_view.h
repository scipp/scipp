// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/element_array_view.h"

namespace scipp::core {

struct bucket_base {
  using range_type = std::pair<scipp::index, scipp::index>;
};
template <class T> struct bucket : bucket_base {
  using element_type = typename T::view_type;
  using const_element_type = typename T::const_view_type;
};

template <class T>
class bucket_array_view
    : public ElementArrayView<const bucket_base::range_type> {
public:
  using value_type = const typename bucket_base::range_type;
  using base = ElementArrayView<const value_type>;

  bucket_array_view(value_type *buckets, const scipp::index offset,
                    const Dimensions &iterDims, const Dimensions &dataDims,
                    const Dim dim, T &buffer)
      : ElementArrayView<value_type>(buckets, offset, iterDims, dataDims),
        m_transform{dim, &buffer} {}

  auto begin() const {
    return boost::make_transform_iterator(base::begin(), m_transform);
  }
  auto end() const {
    return boost::make_transform_iterator(base::end(), m_transform);
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
  using bucket_array_view<T>::bucket_array_view;
};

/// Specialization of ElementArrayView for const access to bucketed data.
///
/// Iteration returns a const view to a slice of the underlying buffer. For
/// example, a VariableConstView in case of T=Variable.
template <class T>
class ElementArrayView<const bucket<T>> : public bucket_array_view<const T> {
  using bucket_array_view<const T>::bucket_array_view;
};

} // namespace scipp::core

namespace scipp {
using core::bucket;
}
