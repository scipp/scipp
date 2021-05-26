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
  const auto &bin_param = param.bucketParams();
  return bin_param ? bin_param.dims : get_nested_dims(params...);
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

  /// Construct without bins.
  template <class... StridesArgs>
  explicit MultiIndex(const Dimensions &iter_dims,
                      const StridesArgs &... strides)
      : m_inner_ndim{iter_dims.ndim()}, m_ndim{iter_dims.ndim()} {
    scipp::index d = iter_dims.ndim() - 1;
    for (const auto size : iter_dims.shape()) {
      m_shape[d--] = size;
    }
    detail::copy_strides(m_stride, m_inner_ndim,
                         std::index_sequence_for<StridesArgs...>(), strides...);
  }

public:
  /// Determine from arguments if binned.
  template <class... Params>
  explicit MultiIndex(const ElementArrayViewParams &param,
                      const Params &... params)
      : MultiIndex{
            (!param.bucketParams() && (!params.bucketParams() && ...))
                ? MultiIndex(param.dims(), param.strides(), params.strides()...)
                : MultiIndex{binned_tag{},
                             detail::get_nested_dims(param, params...), param,
                             params...}} {}

private:
  struct binned_tag {};

  /// Construct with bins.
  template <class... Params>
  explicit MultiIndex(binned_tag, const Dimensions &inner_dims,
                      const Params &... params)
      : m_inner_ndim{inner_dims.ndim()} {
    std::reverse_copy(inner_dims.shape().begin(), inner_dims.shape().end(),
                      m_shape.begin());

    detail::copy_strides(
        m_stride, inner_dims.ndim(), std::index_sequence_for<Params...>(),
        params.bucketParams() ? Strides{inner_dims} : Strides{}...);
    const auto bin_dims = detail::get_head(params...).dims();

    m_ndim = m_inner_ndim + bin_dims.ndim();
    m_bin = std::array{BinIterator(params)...};
    detail::validate_bin_indices(params...);
    const auto nested_dims = detail::get_nested_dims(params...);
    const Dim slice_dim = detail::get_slice_dim(params.bucketParams()...);
    m_nested_stride = nested_dims.offset(slice_dim);
    m_nested_dim_index = m_inner_ndim - nested_dims.index(slice_dim) - 1;

    // TODO needed? can use set_index(0)?
    if (bin_dims.volume() == 0) {
      return; // operands are empty, leave everything below default initialized
    }
    std::reverse_copy(bin_dims.shape().begin(), bin_dims.shape().end(),
                      m_shape.begin() + m_inner_ndim);
    std::array<std::array<scipp::index, N>, NDIM_MAX> binStrides;
    detail::copy_strides(binStrides, bin_ndim(),
                         std::index_sequence_for<Params...>(),
                         params.strides()...);
    for (scipp::index data = 0; data < N; ++data) {
      for (scipp::index d = 0; d < NDIM_MAX - m_inner_ndim; ++d)
        m_stride[m_inner_ndim + d][data] = binStrides[d][data];
      load_bin_params(data);
    }
    if (m_shape[m_nested_dim_index] == 0)
      seek_bin();
  }

public:
  constexpr void increment_outer() noexcept {
    // Go through all nested dims (with bins) / all dims (without bins)
    // where we have reached the end.
    for (scipp::index d = 0; (d < m_inner_ndim - 1) && dim_at_end(d); ++d) {
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
    // Nested dims incremented, move on to bins.
    // Note that we do not check whether there are any bins but whether
    // the outer Variable is scalar because to loop above is enough to set up
    // the coord in that case.
    if (bin_ndim() != 0 && dim_at_end(m_inner_ndim - 1))
      seek_bin();
  }

  constexpr void increment() noexcept {
    for (scipp::index data = 0; data < N; ++data)
      m_data_index[data] += m_stride[0][data];
    ++m_coord[0];
    if (dim_at_end(0))
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
      set_bins_index(index);
    } else {
      extract_indices(index, m_shape.begin(), m_shape.begin() + m_inner_ndim,
                      m_coord.begin());
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] =
            detail::flat_index(data, m_coord, m_stride, 0, m_inner_ndim);
      }
    }
  }

  constexpr void set_to_end() noexcept {
    if (has_bins()) {
      set_to_end_bin();
    } else {
      if (m_inner_ndim == 0) {
        m_coord[0] = 1;
      } else {
        std::fill(m_coord.begin(), m_coord.begin() + m_inner_ndim - 1, 0);
        m_coord[m_inner_ndim - 1] = m_shape[m_inner_ndim - 1];
      }
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] =
            detail::flat_index(data, m_coord, m_stride, 0, m_inner_ndim);
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
    it.set_to_end();
    return it;
  }

  [[nodiscard]] bool has_bins() const noexcept {
    return m_nested_dim_index != -1;
  }

  /// Return true if the first subindex has a 0 stride
  [[nodiscard]] bool has_stride_zero() const noexcept {
    for (scipp::index dim = 0; dim < m_ndim; ++dim)
      if (m_stride[dim][0] == 0)
        return true;
    return false;
  }

private:
  constexpr auto dim_at_end(const scipp::index dim) const noexcept {
    return m_coord[dim] == m_shape[dim];
  }

  constexpr auto bin_ndim() const noexcept { return m_ndim - m_inner_ndim; }

  struct BinIterator {
    BinIterator() = default;
    explicit BinIterator(const ElementArrayViewParams &params)
        : m_indices{params.bucketParams().indices} {}

    [[nodiscard]] bool is_binned() noexcept { return m_indices != nullptr; }

    scipp::index m_bin_index{0};
    const std::pair<scipp::index, scipp::index> *m_indices{nullptr};
  };

  constexpr void set_bins_index(const scipp::index index) noexcept {
    auto coord_it = m_coord.begin();
    std::fill(coord_it, coord_it + m_inner_ndim, 0);
    if (bin_ndim() == 0 && index != 0) {
      m_coord[m_nested_dim_index] = m_shape[m_nested_dim_index];
    } else {
      auto shape_it = m_shape.begin() + m_inner_ndim;
      extract_indices(index, shape_it, shape_it + bin_ndim(),
                      coord_it + m_inner_ndim);
    }

    for (scipp::index data = 0; data < N; ++data) {
      m_bin[data].m_bin_index =
          detail::flat_index(data, m_coord, m_stride, m_inner_ndim, m_ndim);
      load_bin_params(data);
    }
    if (m_shape[m_nested_dim_index] == 0 && !dim_at_end(m_ndim - 1))
      seek_bin();
  }

  constexpr void set_to_end_bin() noexcept {
    auto coord_it = m_coord.begin();
    std::fill(coord_it, coord_it + m_ndim, 0);
    const auto last_dim = (bin_ndim() == 0 ? m_nested_dim_index : m_ndim - 1);
    m_coord[last_dim] = m_shape[last_dim];

    for (scipp::index data = 0; data < N; ++data) {
      // Only one dim contributes, all others have coord = 0.
      m_bin[data].m_bin_index = m_coord[last_dim] * m_stride[last_dim][data];
      load_bin_params(data);
    }
  }

  constexpr void increment_outer_bins() noexcept {
    for (scipp::index dim = m_inner_ndim; (dim < m_ndim - 1) && dim_at_end(dim);
         ++dim) {
      for (scipp::index data = 0; data < N; ++data) {
        m_bin[data].m_bin_index +=
            // take a step in dimension dim+1
            m_stride[dim + 1][data]
            // rewind dimension dim (m_coord[d] == m_shape[d])
            - m_coord[dim] * m_stride[dim][data];
      }
      ++m_coord[dim + 1];
      m_coord[dim] = 0;
    }
  }

  constexpr void increment_bins() noexcept {
    const auto dim = m_inner_ndim;
    for (scipp::index data = 0; data < N; ++data) {
      m_bin[data].m_bin_index += m_stride[dim][data];
    }
    std::fill(m_coord.begin(), m_coord.begin() + m_inner_ndim, 0);
    ++m_coord[dim];
    if (dim_at_end(dim))
      increment_outer_bins();
    if (!dim_at_end(m_ndim - 1)) {
      for (scipp::index data = 0; data < N; ++data) {
        load_bin_params(data);
      }
    }
  }

  constexpr void seek_bin() noexcept {
    do {
      increment_bins();
    } while (m_shape[m_nested_dim_index] == 0 && !dim_at_end(m_ndim - 1));
  }

  constexpr void load_bin_params(const scipp::index data) noexcept {
    if (!m_bin[data].is_binned()) {
      m_data_index[data] =
          detail::flat_index(data, m_coord, m_stride, 0, m_ndim);
    } else if (!dim_at_end(m_ndim - 1)) {
      // All bins are guaranteed to have the same size.
      // Use common m_shape and m_nested_stride for all.
      const auto [begin, end] = m_bin[data].m_indices[m_bin[data].m_bin_index];
      m_shape[m_nested_dim_index] = end - begin;
      m_data_index[data] = m_nested_stride * begin;
    }
    // else: at end of bins
  }

  std::array<scipp::index, N> m_data_index = {};
  // This does *not* 0-init the inner arrays!
  std::array<std::array<scipp::index, N>, NDIM_MAX> m_stride = {};
  std::array<scipp::index, NDIM_MAX> m_coord = {};
  std::array<scipp::index, NDIM_MAX> m_shape = {};
  scipp::index m_inner_ndim{0};
  scipp::index m_ndim{0};
  /// Stride in bins along dim referred to by indices, e.g., 2D bins
  /// slicing along first or second dim.
  scipp::index m_nested_stride = {};
  /// Index of dim referred to by indices to distinguish, e.g., 2D bins
  /// slicing along first or second dim.
  /// -1 if not binned.
  scipp::index m_nested_dim_index{-1};
  std::array<BinIterator, N> m_bin = {};
};

template <class... StridesArgs>
MultiIndex(const Dimensions &, const StridesArgs &...)
    -> MultiIndex<sizeof...(StridesArgs)>;
template <class... Params>
MultiIndex(const ElementArrayViewParams &, const Params &...)
    -> MultiIndex<sizeof...(Params) + 1>;

} // namespace scipp::core
