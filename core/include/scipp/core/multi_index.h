// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <functional>
#include <numeric>
#include <optional>

#include "scipp-core_export.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"

namespace scipp::core {

SCIPP_CORE_EXPORT std::array<scipp::index, NDIM_MAX>
get_strides(const Dimensions &iterDims, const Dimensions &dataDims);

SCIPP_CORE_EXPORT void
validate_bucket_indices_impl(const ElementArrayViewParams &param0,
                             const ElementArrayViewParams &param1);

inline auto get_nested_dims() { return Dimensions(); }
template <class T, class... Ts>
auto get_nested_dims(const T &param, const Ts &... params) {
  return param ? param.dims : get_nested_dims(params...);
}

inline auto get_slice_dim() { return Dim::Invalid; }
template <class T, class... Ts>
auto get_slice_dim(const T &param, const Ts &... params) {
  return param ? param.dim : get_slice_dim(params...);
}

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
    m_ndim = iterDims.ndim();
  }

  template <class... Params>
  MultiIndex(const ElementArrayViewParams &param, const Params &... params) {
    init(param, params...);
  }

  template <class Param> void validate_bucket_indices(const Param &) {}

  /// Check that corresponding buckets have matching sizes.
  template <class Param0, class Param1, class... Params>
  void validate_bucket_indices(const Param0 &param0, const Param1 &param1,
                               const Params &... params) {
    if (param0.bucketParams() && param1.bucketParams())
      validate_bucket_indices_impl(param0, param1);
    if (param0.bucketParams())
      validate_bucket_indices(param0, params...);
    else
      validate_bucket_indices(param1, params...);
  }

  template <class... Params> void init(const Params &... params) {
    const auto iterDims = std::array<Dimensions, N>{params.dims()...}[0];
    if ((!params.bucketParams() && ...)) {
      *this = MultiIndex(iterDims, params.dataDims()...);
      return;
    }
    validate_bucket_indices(params...);
    m_bucket = std::array{BucketIterator(params)...};
    const auto nestedDims = get_nested_dims(params.bucketParams()...);
    const Dim sliceDim = get_slice_dim(params.bucketParams()...);
    m_ndim_nested = nestedDims.ndim();
    m_ndim = iterDims.ndim() + m_ndim_nested;
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
    if (m_shape[m_nested_dim_index] == 0)
      seek_bucket();
  }

  constexpr void load_bucket_params(const scipp::index i) noexcept {
    if (m_bucket[i].m_bucket_index >= m_bucket[i].m_size)
      return; // at end or dense
    // All bins are guaranteed to have the same size.
    // Use common m_shape and m_nested_stride for all.
    const auto [begin, end] = m_bucket[i].m_indices[m_bucket[i].m_bucket_index];
    m_shape[m_nested_dim_index] = end - begin;
    m_data_index[i] = m_nested_stride * begin;
  }

  constexpr void seek_bucket() noexcept {
    do {
      // go through bin dims which have reached their end (including last
      // pre-bin dim)
      for (scipp::index d = m_ndim_nested - 1;
           (m_coord[d] == m_shape[d]) && (d < NDIM_MAX - 1); ++d) {
        for (scipp::index data = 0; data < N; ++data) {
          m_data_index[data] +=
              // take a step in dimension d+1
              m_stride[data][d + 1]
              // rewind dimension d (m_coord[d] == m_shape[d])
              - m_coord[d] * m_stride[data][d];
          // move to next bucket
          if (d == m_ndim_nested - 1) // last non-bin dimension
            m_bucket[data].m_bucket_index += m_stride[data][d + 1];
          else // bin dimension -> rewind earlier bins
            m_bucket[data].m_bucket_index +=
                m_stride[data][d + 1] - m_coord[d] * m_stride[data][d];
          load_bucket_params(data);
        }
        ++m_coord[d + 1];
        m_coord[d] = 0;
      }
    } while (m_shape[m_nested_dim_index] == 0 && m_coord[m_ndim] == 0);
  }

  constexpr void increment_outer() noexcept {
    // Go through all nested dims (with bins) / all dims (without bins)
    // where we have reached the end.
    for (scipp::index d = 0;
         (m_coord[d] == m_shape[d]) && (d < m_ndim_nested - 1); ++d) {
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] +=
            // take a step in dimension d+1
            m_stride[data][d + 1]
            // rewind dimension d (m_coord[d] == m_shape[d])
            - m_coord[d] * m_stride[data][d];
      }
      ++m_coord[d + 1];
      m_coord[d] = 0;
    }
    // nested dims incremented, move on to bins
    seek_bucket();
  }

  constexpr void increment() noexcept {
    for (scipp::index data = 0; data < N; ++data)
      m_data_index[data] += m_stride[data][0];
    ++m_coord[0];
    if (m_coord[0] == m_shape[0])
      increment_outer();
  }

  constexpr void increment_innermost() noexcept {
    for (scipp::index data = 0; data < N; ++data)
      m_data_index[data] += m_stride[data][0];
  }

  [[nodiscard]] scipp::index n_contiguous_dims() const noexcept {
    if (has_bins()) {
      return 0; // TODO can do better for bins?
    }
    scipp::index needed_stride = 1;
    for (scipp::index dim = 0; dim < m_ndim; ++dim) {
      for (scipp::index data = 0; data < N; ++data) {
        if (m_stride[data][dim] != needed_stride) {
          return dim;
        }
      }
      needed_stride *= m_shape[dim];
    }
    return m_ndim;
  }

  [[nodiscard]] auto innermost_strides() const noexcept {
    std::array<scipp::index, N> strides;
    for (scipp::index data = 0; data < N; ++data) {
      strides[data] = m_stride[data][0];
    }
    return strides;
  }

  [[nodiscard]] auto inner_strides() const noexcept {
    const auto strides = innermost_strides();
    if (std::any_of(strides.begin(), strides.end(),
                    [](scipp::index s) { return s > 1; })) {
      return std::tuple(scipp::index{0}, strides);
    }
    // TODO (0,0,...0) allowed?

    const scipp::index ndim_inner_max = has_bins() ? m_ndim_nested : m_ndim;
    scipp::index ndim_inner = 1;
    std::array<scipp::index, N> needed_strides = strides;
    for (; ndim_inner < ndim_inner_max; ++ndim_inner) {
      for (scipp::index data = 0; data < N; ++data) {
        // This assumes that strides[data] in {0, 1}.
        needed_strides[data] *=
            strides[data] == 0 ? 0 : m_shape[ndim_inner - 1];
        if (m_stride[data][ndim_inner] != needed_strides[data]) {
          return std::tuple(ndim_inner, strides);
        }
      }
    }
    return std::tuple(ndim_inner, strides);
  }

  /// Set the absolute index. In the special case of iteration with buckets,
  /// this sets the *index of the bucket* and NOT the full index within the
  /// iterated data.
  constexpr void set_index(const scipp::index offset) noexcept {
    auto remainder{offset};
    for (scipp::index d = 0; d < NDIM_MAX; ++d) {
      if (has_bins() && d < m_ndim_nested) {
        m_coord[d] = 0;
      } else {
        if (m_shape[d] == 0) {
          m_coord[d] = remainder; // this serves as the end index
          break;
        }
        m_coord[d] = remainder % m_shape[d];
        remainder /= m_shape[d];
      }
    }
    for (scipp::index data = 0; data < N; ++data) {
      m_data_index[data] = 0;
      for (scipp::index d = 0; d < NDIM_MAX; ++d)
        m_data_index[data] += m_stride[data][d] * m_coord[d];
    }
    if (has_bins()) {
      for (scipp::index data = 0; data < N; ++data) {
        m_bucket[data].m_bucket_index = 0;
        for (scipp::index d = m_ndim_nested; d < NDIM_MAX; ++d)
          m_bucket[data].m_bucket_index += m_stride[data][d] * m_coord[d];
        load_bucket_params(data);
      }
      if (m_shape[m_nested_dim_index] == 0 && offset != m_end_sentinel)
        seek_bucket();
    }
  }

  constexpr auto get() const noexcept { return m_data_index; }

  constexpr bool operator==(const MultiIndex &other) const noexcept {
    return m_coord == other.m_coord;
  }
  constexpr bool operator!=(const MultiIndex &other) const noexcept {
    return m_coord != other.m_coord;
  }

  [[nodiscard]] constexpr scipp::index ndim() const noexcept { return m_ndim; }

  [[nodiscard]] constexpr auto shape() const noexcept { return m_shape; }

  [[nodiscard]] constexpr auto
  volume(const std::optional<scipp::index> ndim) const noexcept {
    return std::accumulate(
        m_shape.begin(),
        std::next(m_shape.begin(), std::min(ndim.value_or(m_ndim), m_ndim)), 1,
        std::multiplies<scipp::index>{});
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

  [[nodiscard]] scipp::index end_sentinel() const noexcept {
    return m_end_sentinel;
  }

  [[nodiscard]] bool has_bins() const noexcept {
    return m_ndim_nested != NDIM_MAX;
  }

  /// Return true if the first subindex has a 0 stride
  [[nodiscard]] bool has_stride_zero() const noexcept {
    for (scipp::index i = 0; i < m_ndim; ++i)
      if (m_stride[0][i] == 0)
        return true;
    return false;
  }

private:
  struct BucketIterator {
    BucketIterator() = default;
    BucketIterator(const ElementArrayViewParams &params) {
      m_indices = params.bucketParams().indices;
      m_size = params.bucketParams() ? params.dataDims().volume() : 0;
    }
    scipp::index m_bucket_index{0};
    const std::pair<scipp::index, scipp::index> *m_indices{nullptr};
    scipp::index m_size{0};
  };
  std::array<scipp::index, N> m_data_index = {};
  std::array<std::array<scipp::index, NDIM_MAX>, N> m_stride = {};
  std::array<scipp::index, NDIM_MAX> m_coord = {};
  std::array<scipp::index, NDIM_MAX> m_shape = {};
  /// End-sentinel, essentially the volume of the iteration dimensions.
  scipp::index m_end_sentinel{1};
  /// Number of dimensions within bucket, NDIM_MAX if no buckets.
  scipp::index m_ndim_nested{NDIM_MAX};
  /// Stride in buckets along dim referred to by indices, e.g., 2D buckets
  /// slicing along first or second dim.
  scipp::index m_nested_stride = {};
  /// Index of dim referred to by indices to distinguish, e.g., 2D buckets
  /// slicing along first or second dim.
  scipp::index m_nested_dim_index = {};
  std::array<BucketIterator, N> m_bucket = {};
  scipp::index m_ndim;
};
template <class... DataDims>
MultiIndex(const Dimensions &, DataDims &...)
    -> MultiIndex<sizeof...(DataDims)>;
template <class... Params>
MultiIndex(const ElementArrayViewParams &, const Params &...)
    -> MultiIndex<sizeof...(Params) + 1>;

} // namespace scipp::core
