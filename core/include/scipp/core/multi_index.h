// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/core/dimensions.h"

namespace scipp::core {

template <class T> scipp::index to_linear(const T &coord, const T &shape) {
  scipp::index linear = coord.front();
  for (scipp::index d = 1; d < scipp::size(coord); ++d)
    linear = shape[d] * linear + coord[d];
  return linear;
}

/// Strides in dataDims when iterating iterDims.
auto get_strides(const Dimensions &iterDims, const Dimensions &dataDims) {
  std::array<scipp::index, NDIM_MAX> strides = {};
  scipp::index d = iterDims.ndim() - 1;
  for (const auto dim : iterDims.labels()) {
    if (dataDims.contains(dim))
      strides[d--] = dataDims.offset(dim);
    else
      strides[d--] = 0;
  }
  return strides;
}

template <class... DataDims> class SCIPP_CORE_EXPORT MultiIndex {
public:
  constexpr static scipp::index N = sizeof...(DataDims);
  MultiIndex(const Dimensions &iterDims, const DataDims &... dataDims) {
    scipp::index d = iterDims.ndim() - 1;
    for (const auto size : iterDims.shape())
      m_shape[d--] = size;
    m_stride = std::array<std::array<scipp::index, NDIM_MAX>, N>{
        get_strides(iterDims, dataDims)...};
  }

  constexpr void increment_outer() noexcept {
    scipp::index d = 0;
    while ((m_coord[d] == m_shape[d]) && (d < NDIM_MAX - 1)) {
      for (scipp::index data = 0; data < N; ++data)
        m_data_index[data] +=
            m_stride[data][d + 1] - m_coord[d] * m_stride[data][d];
      ++m_coord[d + 1];
      m_coord[d] = 0;
      ++d;
    }
  }

  constexpr void increment() noexcept {
    for (scipp::index data = 0; data < N; ++data)
      m_data_index[data] += m_stride[data][0];
    ++m_coord[0];
    if (m_coord[0] == m_shape[0])
      increment_outer();
    ++m_iter_index;
  }

  constexpr auto get() const noexcept { return m_data_index; }
  constexpr scipp::index index() const noexcept { return m_iter_index; }

  constexpr bool operator==(const MultiIndex &other) const noexcept {
    return m_iter_index == other.m_iter_index;
  }
  constexpr bool operator!=(const MultiIndex &other) const noexcept {
    return m_iter_index != other.m_iter_index;
  }

private:
  std::array<scipp::index, N> m_data_index = {};
  scipp::index m_iter_index{0};
  std::array<std::array<scipp::index, NDIM_MAX>, N> m_stride = {};
  std::array<scipp::index, NDIM_MAX> m_coord = {};
  std::array<scipp::index, NDIM_MAX> m_shape = {};
};

} // namespace scipp::core
