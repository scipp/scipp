// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <functional>
#include <memory>
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

template <scipp::index N_operands>
constexpr auto get_buffer_size(const scipp::index ndim) noexcept {
  return (N_operands + 2) * std::max(ndim, scipp::index{2});
}

template <size_t... I, class... StridesArgs>
void copy_strides(scipp::index *const dest, const scipp::index ndim,
                  const scipp::index first_dim, std::index_sequence<I...>,
                  const StridesArgs &... strides) {
  // TODO pointer arithmetic
  (
      [&]() {
        for (scipp::index dim = 0; dim < ndim; ++dim) {
          dest[I + (first_dim + dim) * sizeof...(I)] = strides[ndim - 1 - dim];
        }
      }(),
      ...);
}

template <size_t N>
scipp::index
flat_index(const scipp::index i_data, const scipp::index *const coord,
           const scipp::index *const stride, scipp::index begin_index,
           const scipp::index end_index) {
  scipp::index res = 0;
  for (; begin_index < end_index; ++begin_index) {
    // TODO pointer arithemtic
    res += coord[begin_index] * stride[i_data + begin_index * N];
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
      : m_buffer{std::make_unique<scipp::index[]>(
            detail::get_buffer_size<N>(iter_dims.ndim()))},
        m_inner_ndim{iter_dims.ndim()}, m_ndim{iter_dims.ndim()} {
    detail::copy_strides(&stride(0, 0), m_inner_ndim, 0,
                         std::index_sequence_for<StridesArgs...>(), strides...);
    std::reverse_copy(iter_dims.shape().begin(), iter_dims.shape().end(),
                      m_shape.begin());
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
                             detail::get_nested_dims(param, params...),
                             param.dims(), param, params...}} {}

private:
  /// Use to disambiguate between constructors.
  struct binned_tag {};

  /// Construct with bins.
  template <class... Params>
  explicit MultiIndex(binned_tag, const Dimensions &inner_dims,
                      const Dimensions &bin_dims, const Params &... params)
      : m_buffer{std::make_unique<scipp::index[]>(
            detail::get_buffer_size<N>(inner_dims.ndim() + bin_dims.ndim()))},
        m_inner_ndim{inner_dims.ndim()}, m_ndim{inner_dims.ndim() +
                                                bin_dims.ndim()},
        m_bin{BinIterator(params)...} {
    detail::validate_bin_indices(params...);

    detail::copy_strides(
        &stride(0, 0), m_inner_ndim, 0, std::index_sequence_for<Params...>(),
        params.bucketParams() ? Strides{inner_dims} : Strides{}...);
    detail::copy_strides(&stride(0, 0), bin_ndim(), m_inner_ndim,
                         std::index_sequence_for<Params...>(),
                         params.strides()...);
    std::reverse_copy(inner_dims.shape().begin(), inner_dims.shape().end(),
                      m_shape.begin());
    std::reverse_copy(bin_dims.shape().begin(), bin_dims.shape().end(),
                      m_shape.begin() + m_inner_ndim);

    const Dim slice_dim = detail::get_slice_dim(params.bucketParams()...);
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    m_nested_stride = inner_dims.offset(slice_dim);
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    m_nested_dim_index = m_inner_ndim - inner_dims.index(slice_dim) - 1;

    if (bin_dims.volume() == 0) {
      return; // Operands are empty, there are no bins to load.
    }
    for (scipp::index data = 0; data < N; ++data) {
      load_bin_params(data);
    }
    if (m_shape[m_nested_dim_index] == 0)
      seek_bin();
  }

public:
  MultiIndex(const MultiIndex &other)
      : m_buffer{other.copy_buffer()}, m_data_index{other.m_data_index},
        m_shape{other.m_shape}, m_inner_ndim{other.m_inner_ndim},
        m_ndim{other.m_ndim}, m_nested_stride{other.m_nested_stride},
        m_nested_dim_index{other.m_nested_dim_index}, m_bin{other.m_bin} {}

  MultiIndex(MultiIndex &&) noexcept = default;

  MultiIndex &operator=(const MultiIndex &other) {
    m_buffer = other.copy_buffer();
    m_data_index = other.m_data_index;
    m_shape = other.m_shape;
    m_inner_ndim = other.m_inner_ndim;
    m_ndim = other.m_ndim;
    m_nested_stride = other.m_nested_stride;
    m_nested_dim_index = other.m_nested_dim_index;
    m_bin = other.m_bin;
  }

  MultiIndex &operator=(MultiIndex &&) noexcept = default;

  ~MultiIndex() noexcept = default;

  constexpr void increment_outer() noexcept {
    // Go through all nested dims (with bins) / all dims (without bins)
    // where we have reached the end.
    for (scipp::index d = 0; (d < m_inner_ndim - 1) && dim_at_end(d); ++d) {
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] +=
            // take a step in dimension d+1
            stride(d + 1, data)
            // rewind dimension d (coord(d) == m_shape[d])
            - coord(d) * stride(d, data);
      }
      ++coord(d + 1);
      coord(d) = 0;
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
      m_data_index[data] += stride(0, data);
    ++coord(0);
    if (dim_at_end(0))
      increment_outer();
  }

  constexpr void increment_inner_by(const scipp::index distance) noexcept {
    for (scipp::index data = 0; data < N; ++data) {
      m_data_index[data] += distance * stride(0, data);
    }
    coord(0) += distance;
  }

  [[nodiscard]] auto inner_strides() const noexcept {
    // TODO
    std::array<scipp::index, N> aux;
    std::copy(&stride(0, 0), &stride(0, 0) + N, aux.begin());
    return aux;
  }

  [[nodiscard]] constexpr scipp::index inner_distance_to_end() const noexcept {
    return m_shape[0] - coord(0);
  }

  [[nodiscard]] constexpr scipp::index
  inner_distance_to(const MultiIndex &other) const noexcept {
    return other.coord(0) - coord(0);
  }

  /// Set the absolute index. In the special case of iteration with bins,
  /// this sets the *index of the bin* and NOT the full index within the
  /// iterated data.
  constexpr void set_index(const scipp::index index) noexcept {
    if (has_bins()) {
      set_bins_index(index);
    } else {
      extract_indices(index, m_shape.begin(), m_shape.begin() + m_inner_ndim,
                      &coord(0));
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] = detail::flat_index<N>(
            data, &coord(0), &stride(0, 0), 0, m_inner_ndim);
      }
    }
  }

  constexpr void set_to_end() noexcept {
    if (has_bins()) {
      set_to_end_bin();
    } else {
      if (m_inner_ndim == 0) {
        coord(0) = 1;
      } else {
        std::fill(&coord(0), &coord(0) + m_inner_ndim - 1, 0);
        coord(m_inner_ndim - 1) = m_shape[m_inner_ndim - 1];
      }
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] = detail::flat_index<N>(
            data, &coord(0), &stride(0, 0), 0, m_inner_ndim);
      }
    }
  }

  constexpr auto get() const noexcept { return m_data_index; }

  constexpr bool operator==(const MultiIndex &other) const noexcept {
    return std::equal(&coord(0), &coord(0) + std::max(m_ndim, scipp::index{2}),
                      &other.coord(0));
  }
  constexpr bool operator!=(const MultiIndex &other) const noexcept {
    return !(*this == other);
  }

  constexpr bool in_same_chunk(const MultiIndex &other,
                               const scipp::index first_dim) const noexcept {
    for (scipp::index dim = first_dim; dim < m_ndim; ++dim) {
      if (coord(dim) != other.coord(dim)) {
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
      if (stride(dim, 0) == 0)
        return true;
    return false;
  }

private:
  constexpr auto dim_at_end(const scipp::index dim) const noexcept {
    return coord(dim) == std::max(m_shape[dim], scipp::index{1});
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
    std::fill(&coord(0), &coord(0) + m_inner_ndim, 0);
    if (bin_ndim() == 0 && index != 0) {
      coord(m_nested_dim_index) = m_shape[m_nested_dim_index];
    } else {
      auto shape_it = m_shape.begin() + m_inner_ndim;
      extract_indices(index, shape_it, shape_it + bin_ndim(),
                      &coord(0) + m_inner_ndim);
    }

    for (scipp::index data = 0; data < N; ++data) {
      m_bin[data].m_bin_index = detail::flat_index<N>(
          data, &coord(0), &stride(0, 0), m_inner_ndim, m_ndim);
      load_bin_params(data);
    }
    if (m_shape[m_nested_dim_index] == 0 && !dim_at_end(m_ndim - 1))
      seek_bin();
  }

  constexpr void set_to_end_bin() noexcept {
    std::fill(&coord(0), &coord(0) + m_ndim, 0);
    const auto last_dim = (bin_ndim() == 0 ? m_nested_dim_index : m_ndim - 1);
    coord(last_dim) = m_shape[last_dim];

    for (scipp::index data = 0; data < N; ++data) {
      // Only one dim contributes, all others have coord = 0.
      m_bin[data].m_bin_index = coord(last_dim) * stride(last_dim, data);
      load_bin_params(data);
    }
  }

  constexpr void increment_outer_bins() noexcept {
    for (scipp::index dim = m_inner_ndim; (dim < m_ndim - 1) && dim_at_end(dim);
         ++dim) {
      for (scipp::index data = 0; data < N; ++data) {
        m_bin[data].m_bin_index +=
            // take a step in dimension dim+1
            stride(dim + 1, data)
            // rewind dimension dim (coord(d) == m_shape[d])
            - coord(dim) * stride(dim, data);
      }
      ++coord(dim + 1);
      coord(dim) = 0;
    }
  }

  constexpr void increment_bins() noexcept {
    const auto dim = m_inner_ndim;
    for (scipp::index data = 0; data < N; ++data) {
      m_bin[data].m_bin_index += stride(dim, data);
    }
    std::fill(&coord(0), &coord(0) + m_inner_ndim, 0);
    ++coord(dim);
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
          detail::flat_index<N>(data, &coord(0), &stride(0, 0), 0, m_ndim);
    } else if (!dim_at_end(m_ndim - 1)) {
      // All bins are guaranteed to have the same size.
      // Use common m_shape and m_nested_stride for all.
      const auto [begin, end] = m_bin[data].m_indices[m_bin[data].m_bin_index];
      m_shape[m_nested_dim_index] = end - begin;
      m_data_index[data] = m_nested_stride * begin;
    }
    // else: at end of bins
  }

  /// Stride for each operand in each dimension.
  [[nodiscard]] scipp::index &stride(const scipp::index dim,
                                     const scipp::index data) noexcept {
    return m_buffer[data + dim * N];
  }

  [[nodiscard]] const scipp::index &
  stride(const scipp::index dim, const scipp::index data) const noexcept {
    return m_buffer[data + dim * N];
  }

  /// Current index in iteration dimensions for both bin and inner dims.
  [[nodiscard]] scipp::index &coord(const scipp::index dim) noexcept {
    return m_buffer[std::max(m_ndim, scipp::index{2}) * N + dim];
  }

  [[nodiscard]] const scipp::index &
  coord(const scipp::index dim) const noexcept {
    return m_buffer[std::max(m_ndim, scipp::index{2}) * N + dim];
  }

  auto copy_buffer() const {
    const auto size = detail::get_buffer_size<N>(m_ndim);
    auto new_buffer = std::make_unique<scipp::index[]>(size);
    std::copy(m_buffer.get(), m_buffer.get() + size, new_buffer.get());
    return new_buffer;
  }

  std::unique_ptr<scipp::index[]> m_buffer;

  /// Current flat index into the operands.
  std::array<scipp::index, N> m_data_index = {};
  /// Shape of the iteration dimensions for both bin and inner dims.
  std::array<scipp::index, NDIM_MAX> m_shape = {};
  /// Number of dense dimensions, i.e. same as m_ndim when not binned,
  /// else number of dims in bins.
  scipp::index m_inner_ndim{0};
  /// Total number of dimensions.
  scipp::index m_ndim{0};
  /// Stride in bins along dim referred to by indices, e.g., 2D bins
  /// slicing along first or second dim.
  scipp::index m_nested_stride = {};
  /// Index of dim referred to by indices to distinguish, e.g., 2D bins
  /// slicing along first or second dim.
  /// -1 if not binned.
  scipp::index m_nested_dim_index{-1};
  /// Parameters of the currently loaded bins.
  std::array<BinIterator, N> m_bin = {};
};

template <class... StridesArgs>
MultiIndex(const Dimensions &, const StridesArgs &...)
    -> MultiIndex<sizeof...(StridesArgs)>;
template <class... Params>
MultiIndex(const ElementArrayViewParams &, const Params &...)
    -> MultiIndex<sizeof...(Params) + 1>;

} // namespace scipp::core
