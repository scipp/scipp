// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <functional>
#include <numeric>
#include <optional>

#include "scipp/common/index_composition.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"

namespace scipp::core {
namespace detail {
inline auto get_nested_dims() { return Dimensions(); }

template <class T, class... Ts>
auto get_nested_dims(const T &param, const Ts &...params) {
  const auto &bin_param = param.bucketParams();
  return bin_param ? bin_param.dims : get_nested_dims(params...);
}
} // namespace detail

template <scipp::index N> class MultiIndex {
public:
  /// Determine from arguments if binned.
  template <class... Params>
  explicit MultiIndex(const ElementArrayViewParams &param,
                      const Params &...params)
      : MultiIndex{
            (!param.bucketParams() && (!params.bucketParams() && ...))
                ? MultiIndex(param.dims(), param.strides(), params.strides()...)
                : MultiIndex{binned_tag{},
                             detail::get_nested_dims(param, params...),
                             param.dims(), param,
                             ElementArrayViewParams{params}...}} {}

  /// Construct without bins.
  template <class... StridesArgs>
  explicit MultiIndex(const Dimensions &iter_dims,
                      const StridesArgs &...strides);

private:
  /// Use to disambiguate between constructors.
  struct binned_tag {};

  /// Construct with bins.
  template <class... Params>
  explicit MultiIndex(binned_tag, const Dimensions &inner_dims,
                      const Dimensions &bin_dims, const Params &...params);

public:
  void increment_outer() noexcept {
    for (scipp::index dim = 0; dim < m_ndim - 1 && dim_at_end(dim); ++dim) {
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] +=
            // take a step in dimension dim+1
            m_stride[dim + 1][data]
            // rewind dimension dim (coord(d) == m_shape[d])
            - m_coord[dim] * m_stride[dim][data];
      }
      ++m_coord[dim + 1];
      m_coord[dim] = 0;
    }
  }

  void increment() noexcept {
    for (scipp::index data = 0; data < N; ++data)
      m_data_index[data] += m_stride[0][data];
    ++m_coord[0];
    if (dim_at_end(0))
      increment_outer();
  }

  void increment_by(const scipp::index inner_distance) noexcept {
    for (scipp::index data = 0; data < N; ++data) {
      m_data_index[data] += inner_distance * m_stride[0][data];
    }
    m_coord[0] += inner_distance;
    if (dim_at_end(0))
      increment_outer();
  }

  [[nodiscard]] auto inner_strides() const noexcept {
    return scipp::span<const scipp::index>(
        has_bins() ? m_inner_strides.data() : m_stride[0].data(), N);
  }

  [[nodiscard]] scipp::index bin_size() const noexcept {
    const auto [begin, end] =
        m_indices[m_binned_arg][m_data_index[m_binned_arg]];
    return m_bin_size_scale * (end - begin);
  }

  [[nodiscard]] scipp::index inner_distance_to_end() const noexcept {
    return m_shape[0] - m_coord[0];
  }

  [[nodiscard]] scipp::index
  inner_distance_to(const MultiIndex &other) const noexcept {
    return other.m_coord[0] - m_coord[0];
  }

  /// Set the absolute index. In the special case of iteration with bins,
  /// this sets the *index of the bin* and NOT the full index within the
  /// iterated data.
  void set_index(const scipp::index index) noexcept {
    extract_indices(index, shape_it(), shape_it(m_ndim), coord_it());
    for (scipp::index data = 0; data < N; ++data) {
      m_data_index[data] = flat_index(data, 0, m_ndim);
    }
  }

  void set_to_end() noexcept {
    if (m_ndim == 0) {
      m_coord[0] = 1;
    } else {
      zero_out_coords(m_ndim - 1);
      m_coord[m_ndim - 1] = m_shape[m_ndim - 1];
    }
    for (scipp::index data = 0; data < N; ++data) {
      m_data_index[data] = flat_index(data, 0, m_ndim);
    }
  }

  [[nodiscard]] constexpr auto get() const noexcept { return m_data_index; }

  [[nodiscard]] constexpr auto get_deref_binned() const noexcept {
    auto index = m_data_index;
    for (scipp::index data = 0; data < N; ++data) {
      if (m_indices[data] != nullptr)
        index[data] = m_bin_size_scale * m_indices[data][index[data]].first;
    }
    return index;
  }

  bool operator==(const MultiIndex &other) const noexcept {
    // Assuming the number dimensions match to make the check cheaper.
    return m_coord == other.m_coord;
  }

  bool operator!=(const MultiIndex &other) const noexcept {
    return !(*this == other); // NOLINT
  }

  [[nodiscard]] bool
  in_same_chunk(const MultiIndex &other,
                const scipp::index first_dim) const noexcept {
    // Take scalars of bins into account when calculating ndim.
    for (scipp::index dim = first_dim; dim < m_ndim; ++dim) {
      if (m_coord[dim] != other.m_coord[dim]) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] auto begin() const noexcept {
    auto it(*this);
    it.set_index(0);
    return it;
  }

  [[nodiscard]] auto end() const noexcept {
    auto it(*this);
    it.set_to_end();
    return it;
  }

  [[nodiscard]] bool has_bins() const noexcept { return m_bin_size_scale >= 0; }

  /// Return true if the first subindex has a 0 stride
  [[nodiscard]] bool has_stride_zero() const noexcept {
    for (scipp::index dim = 0; dim < m_ndim; ++dim)
      if (m_stride[dim][0] == 0)
        return true;
    return false;
  }

private:
  [[nodiscard]] auto dim_at_end(const scipp::index dim) const noexcept {
    return m_coord[dim] == std::max(m_shape[dim], scipp::index{1});
  }

  scipp::index flat_index(const scipp::index i_data, scipp::index begin_index,
                          const scipp::index end_index) {
    scipp::index res = 0;
    for (; begin_index < end_index; ++begin_index) {
      res += m_coord[begin_index] * m_stride[begin_index][i_data];
    }
    return res;
  }

  void zero_out_coords(const scipp::index ndim) noexcept {
    const auto end = coord_it(ndim);
    for (auto it = coord_it(); it != end; ++it) {
      *it = 0;
    }
  }

  [[nodiscard]] auto coord_it(const scipp::index dim = 0) noexcept {
    return m_coord.begin() + dim;
  }

  [[nodiscard]] auto shape_it(const scipp::index dim = 0) noexcept {
    return std::next(m_shape.begin(), dim);
  }

  [[nodiscard]] auto shape_end() noexcept { return m_shape.begin() + m_ndim; }

  /// Current flat index into the operands.
  std::array<scipp::index, N> m_data_index = {};
  // This does *not* 0-init the inner arrays!
  /// Stride for each operand in each dimension.
  std::array<std::array<scipp::index, N>, NDIM_OP_MAX> m_stride = {};
  /// Current index in iteration dimensions for both bin and inner dims.
  std::array<scipp::index, NDIM_OP_MAX + 1> m_coord = {};
  /// Shape of the iteration dimensions for both bin and inner dims.
  std::array<scipp::index, NDIM_OP_MAX + 1> m_shape = {};
  /// Total number of dimensions.
  scipp::index m_ndim{0};
  /// Scale factor for bin size (1 unless bins are multi-dim).
  scipp::index m_bin_size_scale{-1};
  /// Start/stop indices for binned args, used for translating m_data_index to
  /// index for the bin content.
  std::array<const std::pair<scipp::index, scipp::index> *, N> m_indices = {};
  /// Inner strides in case of iteration with bins
  std::array<scipp::index, N> m_inner_strides = {};
  /// Index of an argument with bins
  scipp::index m_binned_arg{-1};
};

template <class... StridesArgs>
MultiIndex(const Dimensions &, const StridesArgs &...)
    -> MultiIndex<sizeof...(StridesArgs)>;
template <class... Params>
MultiIndex(const ElementArrayViewParams &, const Params &...)
    -> MultiIndex<sizeof...(Params) + 1>;

} // namespace scipp::core
