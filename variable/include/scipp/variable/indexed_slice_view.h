// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include <scipp/units/unit.h>

#include "scipp/variable/shape.h"

namespace scipp::variable {

/// Index-based view of slices of a variable, data array, or dataset.
///
/// The main purpose is to provide common means of handling a collection of
/// slices along a specific dimension. Indices allow for reordering or filtering
/// slices. This class is mainly used for implementing other functionality like
/// `sort` and is typically not used directly.
template <class T> class IndexedSliceView {
private:
  struct make_item {
    const IndexedSliceView *view;
    auto operator()(const scipp::index index) const {
      return view->m_data->slice({view->m_dim, index, index + 1});
    }
  };

public:
  /// Construct view over given data, slicing along `dim` for all given indices.
  IndexedSliceView(T &data, const Dim dim, std::vector<scipp::index> index)
      : m_data(&data), m_dim(dim), m_index(index) {}

  /// Slicing dimension.
  constexpr Dim dim() const noexcept { return m_dim; }
  /// Number of slices.
  constexpr scipp::index size() const noexcept { return m_index.size(); }

  /// The slice with given index.
  auto operator[](const scipp::index index) const {
    return m_data->slice({m_dim, m_index[index], m_index[index] + 1});
  }

  /// Return iterator to the beginning of all slices.
  auto begin() const noexcept {
    return boost::make_transform_iterator(m_index.begin(), make_item{this});
  }
  /// Return iterator to the end of all slices.
  auto end() const noexcept {
    return boost::make_transform_iterator(m_index.end(), make_item{this});
  }

private:
  T *m_data;
  Dim m_dim;
  std::vector<scipp::index> m_index;
};

/// Concatenate all slices of an IndexedSliceView along the view's dimension.
template <class T> auto concatenate(const IndexedSliceView<T> &view) {
  // TODO Recursive implementation a bit like merge sort for better performance.
  // Could also try to find continuous ranges in indices.
  auto out = copy(view[0]);
  for (scipp::index i = 1; i < view.size(); ++i)
    out = concatenate(out, view[i], view.dim());
  return out;
}

} // namespace scipp::variable
