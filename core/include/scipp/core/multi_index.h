// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"

namespace scipp::core {

/// Strides in dataDims when iterating iterDims.
inline auto get_strides(const Dimensions &iterDims,
                        const Dimensions &dataDims) {
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

SCIPP_CORE_EXPORT void
validate_bucket_indices_impl(const element_array_view &param0,
                             const element_array_view &param1);

template <scipp::index N> class SCIPP_CORE_EXPORT MultiIndex {
public:
  template <class... DataDims>
  MultiIndex(const Dimensions &iterDims, DataDims &... dataDims) {
    scipp::index d = iterDims.ndim() - 1;
    for (const auto size : iterDims.shape()) {
      m_shape[d--] = size;
      m_end_sentinel *= size;
    }
    m_stride = std::array<std::array<scipp::index, NDIM_MAX>, N>{
        get_strides(iterDims, dataDims)...};
  }

  template <class... Params>
  MultiIndex(const element_array_view &param, const Params &... params) {
    init(param, params...);
  }

  template <class Param> void validate_bucket_indices(const Param &) {}

  /// Check that corresponding buckets have matching sizes.
  template <class Param0, class Param1, class... Params>
  void validate_bucket_indices(const Param0 &param0, const Param1 &param1,
                               const Params &... params) {
    if (param1.bucketParams())
      validate_bucket_indices_impl(param0, param1);
    validate_bucket_indices(param0, params...);
  }

  template <class... Params> void init(const Params &... params) {
    const auto iterDims = std::array<Dimensions, N>{params.dims()...}[0];
    if ((!params.bucketParams() && ...)) {
      *this = MultiIndex(iterDims, params.dataDims()...);
      return;
    }
    validate_bucket_indices(params...);
    m_bucket = std::array{BucketIterator(params)...};
    const auto nestedDims = std::array{params.bucketParams().dims...}[0];
    const Dim sliceDim = std::array{params.bucketParams().dim...}[0];
    m_ndim_nested = nestedDims.ndim();
    m_nested_stride = nestedDims.offset(sliceDim);
    m_nested_dim_index = m_ndim_nested - nestedDims.index(sliceDim) - 1;
    scipp::index dim = iterDims.ndim() - 1 + nestedDims.ndim();
    for (const auto size : iterDims.shape()) {
      m_shape[dim--] = size;
    }
    for (const auto size : nestedDims.shape()) {
      m_shape[dim--] = size;
    }
    m_stride = std::array<std::array<scipp::index, NDIM_MAX>, N>{get_strides(
        nestedDims, params.bucketParams() ? nestedDims : Dimensions{})...};
    const auto bucketStrides =
        std::array<std::array<scipp::index, NDIM_MAX>, N>{
            get_strides(iterDims, params.dataDims())...};
    for (scipp::index data = 0; data < N; ++data) {
      for (scipp::index d = 0; d < NDIM_MAX - m_ndim_nested; ++d)
        m_stride[data][m_ndim_nested + d] = bucketStrides[data][d];
      load_bucket_params(data);
    }
    m_end_sentinel = iterDims.volume();
  }

  constexpr void load_bucket_params(const scipp::index i) noexcept {
    if (m_bucket[i].m_bucket_index >= m_bucket[i].m_size)
      return; // at end or dense
    const auto [begin, end] = m_bucket[i].m_indices[m_bucket[i].m_bucket_index];
    m_shape[m_nested_dim_index] = end - begin;
    m_data_index[i] = m_nested_stride * begin;
  }

  constexpr void increment_outer() noexcept {
    scipp::index d = 0;
    while ((m_coord[d] == m_shape[d]) && (d < NDIM_MAX - 1)) {
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] +=
            m_stride[data][d + 1] - m_coord[d] * m_stride[data][d];
        if (d + 1 >= m_ndim_nested) {
          // move to next bucket
          if (d == m_ndim_nested - 1)
            m_bucket[data].m_bucket_index += m_stride[data][d + 1];
          else
            m_bucket[data].m_bucket_index +=
                m_stride[data][d + 1] - m_coord[d] * m_stride[data][d];
          load_bucket_params(data);
        }
      }
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
  }

  /// Set the absolute index. In the special case of iteration with buckets,
  /// this sets the *index of the bucket* and NOT the full index within the
  /// iterated data.
  constexpr void set_index(const scipp::index offset) noexcept {
    auto remainder{offset};
    for (scipp::index d = 0; d < NDIM_MAX; ++d) {
      if (m_shape[d] == 0) {
        m_coord[d] = remainder; // this serves as the end index
        break;
      }
      if (m_ndim_nested != NDIM_MAX && d < m_ndim_nested) {
        m_coord[d] = 0;
      } else {
        m_coord[d] = remainder % m_shape[d];
        remainder /= m_shape[d];
      }
    }
    for (scipp::index data = 0; data < N; ++data) {
      m_data_index[data] = 0;
      for (scipp::index d = 0; d < NDIM_MAX; ++d)
        m_data_index[data] += m_stride[data][d] * m_coord[d];
    }
    if (m_ndim_nested != NDIM_MAX) {
      for (scipp::index data = 0; data < N; ++data) {
        m_bucket[data].m_bucket_index = 0;
        for (scipp::index d = m_ndim_nested; d < NDIM_MAX; ++d)
          m_bucket[data].m_bucket_index += m_stride[data][d] * m_coord[d];
        load_bucket_params(data);
      }
    }
  }

  constexpr auto get() const noexcept { return m_data_index; }

  constexpr bool operator==(const MultiIndex &other) const noexcept {
    return m_coord == other.m_coord;
  }
  constexpr bool operator!=(const MultiIndex &other) const noexcept {
    return m_coord != other.m_coord;
  }

  auto begin() const noexcept {
    auto it(*this);
    it.set_index(0);
    return it;
  }

  auto end() const noexcept {
    auto it(*this);
    it.set_index(m_end_sentinel);
    return it;
  }

  // TODO this should be removed, end() can compute volume based on m_coord
  scipp::index end_sentinel() const noexcept { return m_end_sentinel; }

private:
  struct BucketIterator {
    BucketIterator() = default;
    BucketIterator(const element_array_view &params) {
      m_indices = params.bucketParams().indices;
      m_size = params.bucketParams() ? params.dataDims().volume() : 0;
    }
    scipp::index m_bucket_index{0}; // may be different for each array, because
                                    // of slicing/broadcast/transpose
    const std::pair<scipp::index, scipp::index> *m_indices{
        nullptr}; // this is unsliced, only size-match required after
                  // slice/broadcast/transpose, offsets may be different
    scipp::index m_size{0};
  };
  std::array<scipp::index, N> m_data_index = {};
  std::array<std::array<scipp::index, NDIM_MAX>, N> m_stride = {};
  std::array<scipp::index, NDIM_MAX> m_coord = {};
  std::array<scipp::index, NDIM_MAX> m_shape = {};
  scipp::index m_end_sentinel{1};
  scipp::index m_ndim_nested{NDIM_MAX}; // dims of bucket, enforce same... but
                                        // could use to handle dense?
  scipp::index m_nested_stride =
      {}; // same if same dims enforced <- no, not if slice? but buffer is never
          // sliced! only length along bucket slicing dim may differ
  scipp::index m_nested_dim_index = {}; // same if same dims enforce
  std::array<BucketIterator, N> m_bucket = {};
};
template <class... DataDims>
MultiIndex(const Dimensions &, DataDims &...)
    -> MultiIndex<sizeof...(DataDims)>;
template <class... Params>
MultiIndex(const element_array_view &, const Params &...)
    -> MultiIndex<sizeof...(Params) + 1>;

} // namespace scipp::core
