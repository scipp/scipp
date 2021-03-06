// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <functional>
#include <numeric>
#include <optional>

#include "scipp-core_export.h"
#include "scipp/common/index_composition.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"

namespace scipp::core {
SCIPP_CORE_EXPORT void
validate_bin_indices_impl(const ElementArrayViewParams &param0,
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

template <scipp::index N, size_t... I, class... StridesArgs>
void copy_strides(std::array<std::array<scipp::index, NDIM_MAX>, N> &dest,
                  const scipp::index ndim, std::index_sequence<I...>,
                  const StridesArgs &... strides) {
  (std::reverse_copy(strides.begin(), strides.begin() + ndim, dest[I].begin()),
   ...);
  // The last stride must be 0 for the end-state detection.
  if (ndim < NDIM_MAX) {
    ((dest[I][ndim] = 0), ...);
  }
}

template <scipp::index N> class MultiIndex {
public:
  template <class... StridesArgs>
  explicit MultiIndex(const Dimensions &iterDims,
                      const StridesArgs &... strides)
      : m_ndim{iterDims.ndim()} {
    scipp::index d = iterDims.ndim() - 1;
    for (const auto size : iterDims.shape()) {
      m_shape[d--] = size;
      m_end_sentinel *= size;
    }
    copy_strides<N>(m_stride, m_ndim, std::index_sequence_for<StridesArgs...>(),
                    strides...);
  }

  template <class... Params>
  explicit MultiIndex(const ElementArrayViewParams &param,
                      const Params &... params) {
    init(param, params...);
  }

  template <class Param> void validate_bin_indices(const Param &) {}

  /// Check that corresponding bins have matching sizes.
  template <class Param0, class Param1, class... Params>
  void validate_bin_indices(const Param0 &param0, const Param1 &param1,
                            const Params &... params) {
    if (param0.bucketParams() && param1.bucketParams())
      validate_bin_indices_impl(param0, param1);
    if (param0.bucketParams())
      validate_bin_indices(param0, params...);
    else
      validate_bin_indices(param1, params...);
  }

  template <class... Params> void init(const Params &... params) {
    const auto iterDims = std::array<Dimensions, N>{params.dims()...}[0];
    if ((!params.bucketParams() && ...)) {
      *this = MultiIndex(iterDims, params.strides()...);
      return;
    }
    validate_bin_indices(params...);
    const auto nestedDims = get_nested_dims(params.bucketParams()...);
    const Dim sliceDim = get_slice_dim(params.bucketParams()...);
    m_ndim_nested = nestedDims.ndim();
    m_ndim = iterDims.ndim() + m_ndim_nested;
    m_nested_stride = nestedDims.offset(sliceDim);
    m_nested_dim_index = m_ndim_nested - nestedDims.index(sliceDim) - 1;
    m_bin = std::array{BinIterator(params)...};
    scipp::index dim = iterDims.ndim() - 1 + nestedDims.ndim();
    m_end_sentinel = iterDims.volume();
    if (m_end_sentinel == 0) {
      return; // operands are empty, leave everything below default initialized
    }
    for (const auto size : iterDims.shape()) {
      m_shape[dim--] = size;
    }
    for (const auto size : nestedDims.shape()) {
      m_shape[dim--] = size;
    }
    copy_strides<N>(m_stride, nestedDims.ndim(),
                    std::index_sequence_for<Params...>(),
                    params.bucketParams() ? Strides{nestedDims} : Strides{}...);
    std::array<std::array<scipp::index, NDIM_MAX>, N> binStrides;
    copy_strides<N>(binStrides, m_ndim - m_ndim_nested,
                    std::index_sequence_for<Params...>(), params.strides()...);
    for (scipp::index data = 0; data < N; ++data) {
      for (scipp::index d = 0; d < NDIM_MAX - m_ndim_nested; ++d)
        m_stride[data][m_ndim_nested + d] = binStrides[data][d];
      load_bin_params(data);
    }
    if (m_shape[m_nested_dim_index] == 0)
      seek_bin();
  }

  constexpr void load_bin_params(const scipp::index i) noexcept {
    if (m_coord[m_ndim] != 0 || !m_bin[i].is_binned())
      return; // at end or dense
    // All bins are guaranteed to have the same size.
    // Use common m_shape and m_nested_stride for all.
    const auto [begin, end] = m_bin[i].m_indices[m_bin[i].m_bin_index];
    m_shape[m_nested_dim_index] = end - begin;
    m_data_index[i] = m_nested_stride * begin;
  }

  constexpr void seek_bin() noexcept {
    do {
      // go through bin dims which have reached their end (including last
      // pre-bin dim)
      for (scipp::index d = m_ndim_nested - 1;
           (m_coord[d] == m_shape[d]) && (d < NDIM_MAX - 1); ++d) {
        // Increment early so that we can check whether we need to load bins.
        ++m_coord[d + 1];
        for (scipp::index data = 0; data < N; ++data) {
          m_data_index[data] +=
              // take a step in dimension d+1
              m_stride[data][d + 1]
              // rewind dimension d (m_coord[d] == m_shape[d])
              - m_coord[d] * m_stride[data][d];
          // move to next bin
          if (d == m_ndim_nested - 1) // last non-bin dimension
            m_bin[data].m_bin_index += m_stride[data][d + 1];
          else // bin dimension -> rewind earlier bins
            m_bin[data].m_bin_index +=
                m_stride[data][d + 1] - m_coord[d] * m_stride[data][d];
          if (m_coord[d + 1] != m_shape[d + 1]) {
            // We might have hit the end of the data, keep going with the outer
            // dimensions.
            load_bin_params(data);
          }
        }
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
    seek_bin();
  }

  constexpr void increment() noexcept {
    for (scipp::index data = 0; data < N; ++data)
      m_data_index[data] += m_stride[data][0];
    ++m_coord[0];
    if (m_coord[0] == m_shape[0])
      increment_outer();
  }

  constexpr void increment_inner_by(const scipp::index distance) noexcept {
    for (scipp::index data = 0; data < N; ++data) {
      m_data_index[data] += distance * m_stride[data][0];
    }
    m_coord[0] += distance;
  }

  [[nodiscard]] auto inner_strides() const noexcept {
    std::array<scipp::index, N> strides;
    for (scipp::index data = 0; data < N; ++data) {
      strides[data] = m_stride[data][0];
    }
    return strides;
  }

  [[nodiscard]] constexpr scipp::index inner_distance_to_end() const noexcept {
    return m_shape[0] - m_coord[0];
  }

  [[nodiscard]] constexpr scipp::index
  inner_distance_to(const MultiIndex &other) const noexcept {
    return other.m_coord[0] - m_coord[0];
  }

  /// Set the absolute index. In the special case of iteration with bins,
  /// this sets the *index of the bin* and NOT the full index within the
  /// iterated data.
  constexpr void set_index(const scipp::index offset) noexcept {
    auto remainder{offset};
    // This loop is until NDIM_MAX inclusive to handle the end index
    for (scipp::index d = 0; d < NDIM_MAX + 1; ++d) {
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
      m_data_index[data] = flat_index_from_strides(
          m_stride[data].begin(), m_stride[data].begin() + m_ndim,
          m_coord.begin());
    }
    if (has_bins()) {
      for (scipp::index data = 0; data < N; ++data) {
        m_bin[data].m_bin_index = flat_index_from_strides(
            m_stride[data].begin() + m_ndim_nested,
            m_stride[data].begin() + m_ndim, m_coord.begin() + m_ndim_nested);
        load_bin_params(data);
      }
      if (m_shape[m_nested_dim_index] == 0 && offset != m_end_sentinel)
        seek_bin();
    }
  }

  constexpr auto get() const noexcept { return m_data_index; }

  constexpr bool operator==(const MultiIndex &other) const noexcept {
    return m_coord == other.m_coord;
  }
  constexpr bool operator!=(const MultiIndex &other) const noexcept {
    return m_coord != other.m_coord;
  }

  constexpr bool in_same_chunk(const MultiIndex &other,
                               const scipp::index first_dim) const noexcept {
    for (scipp::index dim = first_dim; dim < NDIM_MAX + 1; ++dim) {
      if (m_coord[dim] != other.m_coord[dim]) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] constexpr scipp::index ndim() const noexcept { return m_ndim; }

  [[nodiscard]] constexpr auto shape() const noexcept { return m_shape; }

  [[nodiscard]] constexpr auto inner_size() const noexcept {
    return m_shape[0];
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
    return m_ndim_nested != NDIM_MAX + 1;
  }

  /// Return true if the first subindex has a 0 stride
  [[nodiscard]] bool has_stride_zero() const noexcept {
    for (scipp::index i = 0; i < m_ndim; ++i)
      if (m_stride[0][i] == 0)
        return true;
    return false;
  }

private:
  struct BinIterator {
    BinIterator() = default;
    explicit BinIterator(const ElementArrayViewParams &params)
        : m_indices{params.bucketParams().indices} {}

    [[nodiscard]] bool is_binned() noexcept { return m_indices != nullptr; }

    scipp::index m_bin_index{0};
    const std::pair<scipp::index, scipp::index> *m_indices{nullptr};
  };
  std::array<scipp::index, N> m_data_index = {};
  // This does *not* 0-init the inner arrays!
  std::array<std::array<scipp::index, NDIM_MAX>, N> m_stride = {};
  std::array<scipp::index, NDIM_MAX + 1> m_coord = {};
  std::array<scipp::index, NDIM_MAX + 1> m_shape = {};
  /// End-sentinel, essentially the volume of the iteration dimensions.
  scipp::index m_end_sentinel{1};
  /// Number of dimensions within bin, NDIM_MAX if no bins.
  scipp::index m_ndim_nested{NDIM_MAX + 1};
  /// Stride in bins along dim referred to by indices, e.g., 2D bins
  /// slicing along first or second dim.
  scipp::index m_nested_stride = {};
  /// Index of dim referred to by indices to distinguish, e.g., 2D bins
  /// slicing along first or second dim.
  scipp::index m_nested_dim_index = {};
  std::array<BinIterator, N> m_bin = {};
  scipp::index m_ndim;
};
template <class... StridesArgs>
MultiIndex(const Dimensions &, const StridesArgs &...)
    -> MultiIndex<sizeof...(StridesArgs)>;
template <class... Params>
MultiIndex(const ElementArrayViewParams &, const Params &...)
    -> MultiIndex<sizeof...(Params) + 1>;

} // namespace scipp::core
