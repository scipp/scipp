// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_INDEXED_SLICE_VIEW_H
#define SCIPP_CORE_INDEXED_SLICE_VIEW_H

#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include <scipp/units/unit.h>

namespace scipp::core {

template <class T> class IndexedSliceView {
private:
  struct make_item {
    const IndexedSliceView *view;
    auto operator()(const scipp::index index) const {
      return view->m_data->slice({view->m_dim, index});
    }
  };

public:
  IndexedSliceView(T &data, const Dim dim, std::vector<scipp::index> index)
      : m_data(&data), m_dim(dim), m_index(index) {}

  constexpr Dim dim() const noexcept { return m_dim; }
  constexpr scipp::index size() const noexcept { return m_index.size(); }

  auto operator[](const scipp::index index) const {
    return m_data->slice({m_dim, m_index[index]});
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

} // namespace scipp::core

#endif // SCIPP_CORE_INDEXED_SLICE_VIEW_H
