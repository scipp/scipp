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
namespace detail {
SCIPP_CORE_EXPORT void
validate_bin_indices_impl(const ElementArrayViewParams &param0,
                          const ElementArrayViewParams &param1);

template <class Param> void validate_bin_indices(const Param &) {}

/// Check that corresponding bins have matching sizes.
template <class Param0, class Param1, class... Params>
void validate_bin_indices(const Param0 &param0, const Param1 &param1,
                          const Params &... params) {
  if (param0.bucketParams() && param1.bucketParams())
    detail::validate_bin_indices_impl(param0, param1);
  if (param0.bucketParams())
    validate_bin_indices(param0, params...);
  else
    validate_bin_indices(param1, params...);
}

template <class Head, class... Args>
decltype(auto) get_head(Head &&head, Args &&...) {
  return std::forward<Head>(head);
}

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

template <size_t... I, class... StridesArgs>
void copy_strides(
    std::array<std::array<scipp::index, sizeof...(I)>, NDIM_MAX> &dest,
    const scipp::index ndim, std::index_sequence<I...>,
    const StridesArgs &... strides) {
  (
      [&]() {
        for (scipp::index dim = 0; dim < ndim; ++dim) {
          dest[dim][I] = strides[ndim - 1 - dim];
        }
      }(),
      ...);
  // The last stride must be 0 for the end-state detection.
  if (ndim < NDIM_MAX) {
    ((dest[ndim][I] = 0), ...);
  }
}

template <size_t N>
scipp::index
flat_index(const scipp::index i_data,
           const std::array<scipp::index, NDIM_MAX> &coord,
           const std::array<std::array<scipp::index, N>, NDIM_MAX> &stride,
           scipp::index begin_index, const scipp::index end_index) {
  scipp::index res = 0;
  for (; begin_index < end_index; ++begin_index) {
    res += coord[begin_index] * stride[begin_index][i_data];
  }
  return res;
}
} // namespace detail

template <scipp::index N> class MultiIndex {
public:
  template <class... StridesArgs>
  explicit MultiIndex(const Dimensions &iterDims,
                      const StridesArgs &... strides)
      : m_inner_ndim{iterDims.ndim()} {
    scipp::index d = iterDims.ndim() - 1;
    for (const auto size : iterDims.shape()) {
      m_shape[d--] = size;
      m_end_sentinel *= size;
    }
    detail::copy_strides(m_stride, m_inner_ndim,
                         std::index_sequence_for<StridesArgs...>(), strides...);
  }

  template <class... Params>
  explicit MultiIndex(const ElementArrayViewParams &param,
                      const Params &... params) {
    init(param, params...);
  }

  template <class... Params> void init(const Params &... params) {
    const auto iter_dims = detail::get_head(params...).dims();
    if ((!params.bucketParams() && ...)) {
      *this = MultiIndex(iter_dims, params.strides()...);
      return;
    }

    const auto nested_dims = detail::get_nested_dims(params.bucketParams()...);
    m_has_bins = true;
    m_end_sentinel = iter_dims.volume();

    m_inner_ndim = nested_dims.ndim();
    m_outer_index = BinIndex(*this, params...);
    if (m_end_sentinel == 0) {
      return; // operands are empty, leave everything below default initialized
    }
    std::reverse_copy(nested_dims.shape().begin(), nested_dims.shape().end(),
                      m_shape.begin());
    detail::copy_strides(
        m_stride, nested_dims.ndim(), std::index_sequence_for<Params...>(),
        params.bucketParams() ? Strides{nested_dims} : Strides{}...);
    std::array<std::array<scipp::index, N>, NDIM_MAX> binStrides;
    detail::copy_strides(binStrides, ndim() - m_inner_ndim,
                         std::index_sequence_for<Params...>(),
                         params.strides()...);
    for (scipp::index data = 0; data < N; ++data) {
      for (scipp::index d = 0; d < NDIM_MAX - m_inner_ndim; ++d)
        m_stride[m_inner_ndim + d][data] = binStrides[d][data];
      m_outer_index.load_bin_params(*this, data);
    }
    if (m_shape[m_outer_index.m_nested_dim_index] == 0)
      m_outer_index.seek_bin(*this);
  }

  constexpr void increment_outer() noexcept {
    // Go through all nested dims (with bins) / all dims (without bins)
    // where we have reached the end.
    for (scipp::index d = 0;
         (d < m_inner_ndim - 1) && (m_coord[d] == m_shape[d]); ++d) {
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] +=
            // take a step in dimension d+1
            m_stride[d + 1][data]
            // rewind dimension d (m_coord[d] == m_shape[d])
            - m_coord[d] * m_stride[d][data];
      }
      ++m_coord[d + 1];
      m_coord[d] = 0;
    }
    // nested dims incremented, move on to bins
    // TODO can do without extra test?
    // TODO do not test last dim twice
    if (m_outer_index.m_outer_ndim != 0 &&
        m_coord[m_inner_ndim - 1] == m_shape[m_inner_ndim - 1])
      m_outer_index.seek_for_inc(*this);
  }

  constexpr void increment() noexcept {
    for (scipp::index data = 0; data < N; ++data)
      m_data_index[data] += m_stride[0][data];
    ++m_coord[0];
    if (m_coord[0] == m_shape[0])
      increment_outer();
  }

  constexpr void increment_inner_by(const scipp::index distance) noexcept {
    for (scipp::index data = 0; data < N; ++data) {
      m_data_index[data] += distance * m_stride[0][data];
    }
    m_coord[0] += distance;
  }

  [[nodiscard]] auto &inner_strides() const noexcept { return m_stride[0]; }

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
  constexpr void set_index(const scipp::index index) noexcept {
    if (has_bins()) {
      m_outer_index.set_index(*this, index);
    } else {
      extract_indices(index, m_shape.begin(), m_shape.begin() + m_inner_ndim,
                      m_coord.begin());
      for (scipp::index data = 0; data < N; ++data) {
        // TODO do we need to actually go until m_ndim?
        m_data_index[data] =
            detail::flat_index(data, m_coord, m_stride, 0, m_inner_ndim);
      }
    }
  }

  constexpr auto get() const noexcept { return m_data_index; }

  constexpr auto ndim() const noexcept {
    return m_inner_ndim + m_outer_index.m_outer_ndim;
  };

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

  [[nodiscard]] bool has_bins() const noexcept { return m_has_bins; }

  /// Return true if the first subindex has a 0 stride
  [[nodiscard]] bool has_stride_zero() const noexcept {
    for (scipp::index dim = 0; dim < ndim(); ++dim)
      if (m_stride[dim][0] == 0)
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

  struct BinIndex {
    // TODO remove?
    BinIndex() = default;

    template <class... Params>
    BinIndex(MultiIndex &inner, const Params &... params) {
      const auto iter_dims = detail::get_head(params...).dims();
      detail::validate_bin_indices(params...);
      const auto nested_dims =
          detail::get_nested_dims(params.bucketParams()...);
      const Dim slice_dim = detail::get_slice_dim(params.bucketParams()...);
      m_outer_ndim = iter_dims.ndim();
      m_nested_stride = nested_dims.offset(slice_dim);
      m_nested_dim_index =
          inner.m_inner_ndim - nested_dims.index(slice_dim) - 1;
      m_bin = std::array{BinIterator(params)...};

      // TODO end_sentinel abort

      std::reverse_copy(iter_dims.shape().begin(), iter_dims.shape().end(),
                        inner.m_shape.begin() + inner.m_inner_ndim);
    }

    constexpr void set_index(MultiIndex &inner,
                             const scipp::index index) noexcept {
      auto coord_it = inner.m_coord.begin();
      std::fill(coord_it, coord_it + inner.m_inner_ndim, 0);
      if (m_outer_ndim == 0 && index != 0) {
        inner.m_coord[m_nested_dim_index] = inner.m_shape[m_nested_dim_index];
      } else {
        auto shape_it = inner.m_shape.begin() + inner.m_inner_ndim;
        extract_indices(index, shape_it, shape_it + m_outer_ndim,
                        coord_it + inner.m_inner_ndim);
      }

      for (scipp::index data = 0; data < N; ++data) {
        m_bin[data].m_bin_index =
            detail::flat_index(data, inner.m_coord, inner.m_stride,
                               inner.m_inner_ndim, inner.ndim());
        load_bin_params(inner, data);
      }
      if (inner.m_shape[m_nested_dim_index] == 0 &&
          index != inner.m_end_sentinel)
        seek_bin(inner);
    }

    constexpr void increment_outer(MultiIndex &inner) noexcept {
      for (scipp::index dim = inner.m_inner_ndim;
           (dim < inner.ndim() - 1) &&
           (inner.m_coord[dim] == inner.m_shape[dim]);
           ++dim) {
        // Increment early so that we can check whether we need to load bins.
        ++inner.m_coord[dim + 1];
        for (scipp::index data = 0; data < N; ++data) {
          m_bin[data].m_bin_index +=
              // take a step in dimension dim+1
              inner.m_stride[dim + 1][data]
              // rewind dimension dim (m_coord[d] == m_shape[d])
              - inner.m_coord[dim] * inner.m_stride[dim][data];
        }
        inner.m_coord[dim] = 0;
      }
    }

    constexpr void increment(MultiIndex &inner) noexcept {
      const auto dim = inner.m_inner_ndim;
      for (scipp::index data = 0; data < N; ++data) {
        m_bin[data].m_bin_index += inner.m_stride[dim][data];
      }
      std::fill(inner.m_coord.begin(),
                inner.m_coord.begin() + inner.m_inner_ndim, 0);
      ++inner.m_coord[dim];
      if (inner.m_coord[dim] == inner.m_shape[dim])
        increment_outer(inner);
      if (inner.m_coord[inner.ndim() - 1] != inner.m_shape[inner.ndim() - 1]) {
        // TODO check needed?
        for (scipp::index data = 0; data < N; ++data) {
          load_bin_params(inner, data);
        }
      }
    }

    // TODO name
    constexpr void seek_for_inc(MultiIndex &inner) noexcept {
      do {
        increment(inner);
      } while (inner.m_shape[inner.m_outer_index.m_nested_dim_index] == 0 &&
               inner.m_coord[inner.ndim() - 1] !=
                   inner.m_shape[inner.ndim() - 1]);
    }

    constexpr void load_bin_params(MultiIndex &inner,
                                   const scipp::index data) noexcept {
      // TODO can avoid ndim()?
      if (!m_bin[data].is_binned()) {
        inner.m_data_index[data] = detail::flat_index(
            data, inner.m_coord, inner.m_stride, 0, inner.ndim());
      } else if (inner.m_coord[inner.ndim() - 1] !=
                 inner.m_shape[inner.ndim() - 1]) {
        // All bins are guaranteed to have the same size.
        // Use common m_shape and m_nested_stride for all.
        const auto [begin, end] =
            m_bin[data].m_indices[m_bin[data].m_bin_index];
        inner.m_shape[m_nested_dim_index] = end - begin;
        inner.m_data_index[data] = m_nested_stride * begin;
      }
      // else: at end of bins
    }

    constexpr void seek_bin(MultiIndex &inner) noexcept {
      do {
        // go through bin dims which have reached their end (including last
        // pre-bin dim)
        for (scipp::index d = inner.m_inner_ndim - 1;
             (inner.m_coord[d] == inner.m_shape[d]) && (d < inner.ndim() - 1);
             ++d) {
          // Increment early so that we can check whether we need to load bins.
          ++inner.m_coord[d + 1];
          for (scipp::index data = 0; data < N; ++data) {
            inner.m_data_index[data] +=
                // take a step in dimension d+1
                inner.m_stride[d + 1][data]
                // rewind dimension d (m_coord[d] == m_shape[d])
                - inner.m_coord[d] * inner.m_stride[d][data];
            // move to next bin
            if (d == inner.m_inner_ndim - 1) // last non-bin dimension
              inner.m_outer_index.m_bin[data].m_bin_index +=
                  inner.m_stride[d + 1][data];
            else // bin dimension -> rewind earlier bins
              inner.m_outer_index.m_bin[data].m_bin_index +=
                  inner.m_stride[d + 1][data] -
                  inner.m_coord[d] * inner.m_stride[d][data];
            if (inner.m_coord[d + 1] != inner.m_shape[d + 1]) {
              // We might have hit the end of the data, keep going with the
              // outer dimensions.
              load_bin_params(inner, data);
            }
          }
          inner.m_coord[d] = 0;
        }
      } while (inner.m_shape[inner.m_outer_index.m_nested_dim_index] == 0 &&
               inner.m_coord[inner.ndim() - 1] !=
                   inner.m_shape[inner.ndim() - 1]);
    }

    // TODO store ndim instead?
    scipp::index m_outer_ndim{0};
    /// Stride in bins along dim referred to by indices, e.g., 2D bins
    /// slicing along first or second dim.
    scipp::index m_nested_stride = {};
    /// Index of dim referred to by indices to distinguish, e.g., 2D bins
    /// slicing along first or second dim.
    scipp::index m_nested_dim_index = {};
    std::array<BinIterator, N> m_bin = {};
  };

  std::array<scipp::index, N> m_data_index = {};
  // This does *not* 0-init the inner arrays!
  std::array<std::array<scipp::index, N>, NDIM_MAX> m_stride = {};
  std::array<scipp::index, NDIM_MAX> m_coord = {};
  std::array<scipp::index, NDIM_MAX> m_shape = {};
  /// End-sentinel, essentially the volume of the iteration dimensions.
  scipp::index m_end_sentinel{1};
  scipp::index m_inner_ndim{0};
  BinIndex m_outer_index{};
  // TODO try to remove
  //    -> needed if bins are scalar?
  bool m_has_bins = false;
};

template <class... StridesArgs>
MultiIndex(const Dimensions &, const StridesArgs &...)
    -> MultiIndex<sizeof...(StridesArgs)>;
template <class... Params>
MultiIndex(const ElementArrayViewParams &, const Params &...)
    -> MultiIndex<sizeof...(Params) + 1>;

} // namespace scipp::core
