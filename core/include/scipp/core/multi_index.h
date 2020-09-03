// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/core/dimensions.h"

namespace scipp::core {

struct SCIPP_CORE_EXPORT BucketParams {
  bool operator==(const BucketParams &other) const noexcept {
    return dim == other.dim && dims == other.dims &&
           std::equal(indices.begin(), indices.end(), other.indices.begin(),
                      other.indices.end());
  }
  bool operator!=(const BucketParams &other) const noexcept {
    return !(*this == other);
  }
  explicit operator bool() const noexcept { return *this != BucketParams{}; }
  Dim dim{Dim::Invalid};
  Dimensions dims{};
  scipp::span<const std::pair<scipp::index, scipp::index>> indices{};
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

template <class... DataDims> class SCIPP_CORE_EXPORT MultiIndex {
public:
  constexpr static scipp::index N = sizeof...(DataDims);
  MultiIndex(const Dimensions &iterDims, const DataDims &... dataDims) {
    scipp::index d = iterDims.ndim() - 1;
    for (const auto size : iterDims.shape()) {
      m_shape[d--] = size;
      m_end_sentinel *= size;
    }
    // Last arg is dummy
    m_stride = std::array{get_strides(iterDims, dataDims)...,
                          get_strides(iterDims, iterDims)};
  }

  MultiIndex(const BucketParams &bucket_params, const Dimensions &iterDims,
             const DataDims &... dataDims) {
    const auto &nestedDims = bucket_params.dims;
    m_ndim_nested = nestedDims.ndim();
    m_nested_stride = nestedDims.offset(bucket_params.dim);
    m_nested_dim_index =
        m_ndim_nested - nestedDims.index(bucket_params.dim) - 1;
    m_indices = bucket_params.indices;
    scipp::index dim = iterDims.ndim() - 1 + m_ndim_nested;
    for (const auto size : iterDims.shape()) {
      m_shape[dim--] = size;
    }
    for (const auto size : nestedDims.shape()) {
      m_shape[dim--] = size;
    }
    const auto bucketStrides =
        std::array<std::array<scipp::index, NDIM_MAX>, N>{
            get_strides(iterDims, dataDims)...};
    // TODO Strides must be 0 for dense inputs, need BucketParams for *each*
    // input, unmerged
    // condition is dummy for now (just for expansion)
    const auto nestedStrides =
        std::array<std::array<scipp::index, NDIM_MAX>, N>{get_strides(
            nestedDims,
            dataDims.contains(Dim::Invalid) ? nestedDims : nestedDims)...};
    const auto [begin, end] = m_indices[m_bucket_index];
    for (scipp::index data = 0; data < N; ++data) {
      for (scipp::index d = 0; d < m_ndim_nested; ++d)
        m_stride[data][d] = nestedStrides[data][d];
      for (scipp::index d = 0; d < NDIM_MAX - m_ndim_nested; ++d)
        m_stride[data][m_ndim_nested + d] = bucketStrides[data][d];
      m_stride[data][m_ndim_nested] = 0;
    }
    m_shape[m_nested_dim_index] = end - begin;
    // TODO is output always of maximum shape and has buckets? use index 0
    // instead of N?
    for (scipp::index d = 0; d < m_ndim_nested; ++d)
      m_stride[N][d] = 0;
    for (scipp::index d = 0; d < NDIM_MAX - m_ndim_nested; ++d)
      m_stride[N][m_ndim_nested + d] = bucketStrides[0][d];
    m_end_sentinel = iterDims.volume();
    load_bucket_params();
    // fprintf(stderr, "nested stride %ld nested dim index %ld\n",
    // m_nested_stride,
    //        m_nested_dim_index);
    // fprintf(stderr, "shape %ld %ld %ld\n", m_shape[0], m_shape[1],
    // m_shape[2]); fprintf(stderr, "initial strides %ld %ld %ld\n",
    // m_stride[0][0],
    //        m_stride[0][1], m_stride[0][2]);
    // fprintf(stderr, "initial strides %ld %ld %ld\n", m_stride[N][0],
    //        m_stride[N][1], m_stride[N][2]);
  }

  constexpr void load_bucket_params() noexcept {
    const auto [begin, end] = m_indices[m_bucket_index];
    m_shape[m_nested_dim_index] = end - begin;
    // TODO do not update dense
    for (scipp::index data = 0; data < N; ++data) {
      m_stride[data][m_ndim_nested] =
          m_nested_stride * begin - m_stride[data][m_ndim_nested];
      m_data_index[data] = m_nested_stride * begin;
    }
  }

  constexpr void increment_outer() noexcept {
    scipp::index d = 0;
    while ((m_coord[d] == m_shape[d]) && (d < NDIM_MAX - 1)) {
      if (d + 1 >= m_ndim_nested) {
        // move to next bucket
        m_bucket_index += m_stride[N][d + 1] - m_coord[d] * m_stride[N][d];
        load_bucket_params();
      } else {
        for (scipp::index data = 0; data < N; ++data)
          m_data_index[data] +=
              m_stride[data][d + 1] - m_coord[d] * m_stride[data][d];
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

  constexpr void set_index(const scipp::index offset) noexcept {
    if (m_ndim_nested == NDIM_MAX) {
      auto remainder{offset};
      for (scipp::index d = 0; d < NDIM_MAX; ++d) {
        if (m_shape[d] == 0) {
          m_coord[d] = remainder; // this serves as the end index
          break;
        }
        m_coord[d] = remainder % m_shape[d];
        remainder /= m_shape[d];
      }
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] = 0;
        for (scipp::index d = 0; d < NDIM_MAX; ++d)
          m_data_index[data] += m_stride[data][d] * m_coord[d];
      }
    } else {
      auto remainder{offset};
      for (scipp::index d = 0; d < m_ndim_nested; ++d)
        m_coord[d] = 0;
      for (scipp::index d = m_ndim_nested; d < NDIM_MAX; ++d) {
        if (m_shape[d] == 0) {
          m_coord[d] = remainder; // this serves as the end index
          break;
        }
        m_coord[d] = remainder % m_shape[d];
        remainder /= m_shape[d];
      }
      m_bucket_index = 0;
      for (scipp::index d = m_ndim_nested; d < NDIM_MAX; ++d)
        m_bucket_index += m_stride[N][d] * m_coord[d];
      for (scipp::index data = 0; data < N; ++data) {
        m_stride[data][m_ndim_nested] = 0;
      }
      load_bucket_params();
      // fprintf(stderr, "set_index(%ld) %ld %ld %ld %ld bucket %ld\n", offset,
      //        m_coord[0], m_coord[1], m_coord[2], m_coord[3], m_bucket_index);
      // fprintf(stderr, "strides %ld %ld %ld\n", m_stride[0][0],
      // m_stride[0][1],
      //        m_stride[0][2]);
      // fprintf(stderr, "strides %ld %ld %ld\n", m_stride[N][0],
      // m_stride[N][1],
      //        m_stride[N][2]);
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

  scipp::index end_sentinel() const noexcept { return m_end_sentinel; }

private:
  std::array<scipp::index, N> m_data_index = {};
  std::array<std::array<scipp::index, NDIM_MAX>, N + 1> m_stride = {};
  std::array<scipp::index, NDIM_MAX> m_coord = {};
  std::array<scipp::index, NDIM_MAX> m_shape = {};
  scipp::index m_end_sentinel{1};
  scipp::index m_bucket_index{0};
  scipp::index m_ndim_nested{NDIM_MAX};
  scipp::index m_nested_stride{0};
  scipp::index m_nested_dim_index;
  scipp::span<const std::pair<scipp::index, scipp::index>> m_indices{};

  // BucketParams m_bucket_params;
  // ElementArrayView<const bucket_base::range_type> m_buckets;
  // each bucket corresponds to a slice.
  // iterating slice is always without broadcast or transpose
  // move to next bucket is just changing start and end of slicing dim -> simple
  // "shift" of slice view
};

} // namespace scipp::core
