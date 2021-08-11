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

#ifdef _WIN32
#define SCIPP_CORE_EXPORT_TEMPLATE_DECLARATION
#define SCIPP_CORE_EXPORT_TEMPLATE_DEFINITION SCIPP_CORE_EXPORT
#else
#define SCIPP_CORE_EXPORT_TEMPLATE_DECLARATION SCIPP_CORE_EXPORT
#define SCIPP_CORE_EXPORT_TEMPLATE_DEFINITION
#endif

namespace scipp::core {
namespace detail {
inline auto get_nested_dims() { return Dimensions(); }

template <class T, class... Ts>
auto get_nested_dims(const T &param, const Ts &... params) {
  const auto &bin_param = param.bucketParams();
  return bin_param ? bin_param.dims : get_nested_dims(params...);
}
} // namespace detail

template <scipp::index N>
class SCIPP_CORE_EXPORT_TEMPLATE_DECLARATION MultiIndex {
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
                             param.dims(), param,
                             ElementArrayViewParams{params}...}} {}

  /// Construct without bins.
  template <class... StridesArgs>
  explicit MultiIndex(const Dimensions &iter_dims,
                      const StridesArgs &... strides);

private:
  /// Use to disambiguate between constructors.
  struct binned_tag {};

  /// Construct with bins.
  template <class... Params>
  explicit MultiIndex(binned_tag, const Dimensions &inner_dims,
                      const Dimensions &bin_dims, const Params &... params);

public:
  void increment_outer() noexcept {
    // Go through all nested dims (with bins) / all dims (without bins)
    // where we have reached the end.
    increment_in_dims(
        [this](const scipp::index data) -> scipp::index & {
          return this->m_data_index[data];
        },
        0, m_inner_ndim - 1);
    // Nested dims incremented, move on to bins.
    // Note that we do not check whether there are any bins, instead whether
    // the outer Variable is scalar because the loop above is enough to set up
    // the coord in that case.
    if (bin_ndim() != 0 && dim_at_end(m_inner_ndim - 1))
      seek_bin();
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
    return scipp::span<const scipp::index>(m_stride[0].data(), N);
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
    if (has_bins()) {
      set_bins_index(index);
    } else {
      extract_indices(index, shape_it(), shape_it(m_inner_ndim), coord_it());
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] = flat_index(data, 0, m_inner_ndim);
      }
    }
  }

  void set_to_end() noexcept {
    if (has_bins()) {
      set_to_end_bin();
    } else {
      if (m_inner_ndim == 0) {
        m_coord[0] = 1;
      } else {
        std::fill(coord_it(0), coord_it(m_inner_ndim - 1), 0);
        m_coord[m_inner_ndim - 1] = m_shape[m_inner_ndim - 1];
      }
      for (scipp::index data = 0; data < N; ++data) {
        m_data_index[data] = flat_index(data, 0, m_inner_ndim);
      }
    }
  }

  [[nodiscard]] constexpr auto get() const noexcept { return m_data_index; }

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
  [[nodiscard]] auto dim_at_end(const scipp::index dim) const noexcept {
    return m_coord[dim] == std::max(m_shape[dim], scipp::index{1});
  }

  template <class F>
  void increment_in_dims(const F &data_index, const scipp::index begin_dim,
                         const scipp::index end_dim) {
    for (scipp::index dim = begin_dim; dim < end_dim && dim_at_end(dim);
         ++dim) {
      for (scipp::index data = 0; data < N; ++data) {
        data_index(data) +=
            // take a step in dimension dim+1
            m_stride[dim + 1][data]
            // rewind dimension dim (coord(d) == m_shape[d])
            - m_coord[dim] * m_stride[dim][data];
      }
      ++m_coord[dim + 1];
      m_coord[dim] = 0;
    }
  }

  [[nodiscard]] constexpr auto bin_ndim() const noexcept {
    return m_ndim - m_inner_ndim;
  }

  struct BinIterator {
    BinIterator() = default;
    explicit BinIterator(const ElementArrayViewParams &params)
        : m_indices{params.bucketParams().indices} {}

    [[nodiscard]] bool is_binned() noexcept { return m_indices != nullptr; }

    scipp::index m_bin_index{0};
    const std::pair<scipp::index, scipp::index> *m_indices{nullptr};
  };

  void increment_outer_bins() noexcept {
    increment_in_dims(
        [this](const scipp::index data) -> scipp::index & {
          return this->m_bin[data].m_bin_index;
        },
        m_inner_ndim, m_ndim - 1);
  }

  void increment_bins() noexcept {
    const auto dim = m_inner_ndim;
    for (scipp::index data = 0; data < N; ++data) {
      m_bin[data].m_bin_index += m_stride[dim][data];
    }
    std::fill(coord_it(), coord_it(m_inner_ndim), 0);
    ++m_coord[dim];
    if (dim_at_end(dim))
      increment_outer_bins();
    if (!dim_at_end(m_ndim - 1)) {
      for (scipp::index data = 0; data < N; ++data) {
        load_bin_params(data);
      }
    }
  }

  void seek_bin() noexcept {
    do {
      increment_bins();
    } while (m_shape[m_nested_dim_index] == 0 && !dim_at_end(m_ndim - 1));
  }

  void load_bin_params(const scipp::index data) noexcept {
    if (!m_bin[data].is_binned()) {
      m_data_index[data] = flat_index(data, 0, m_ndim);
    } else if (!dim_at_end(m_ndim - 1)) {
      // All bins are guaranteed to have the same size.
      // Use common m_shape and m_nested_stride for all.
      const auto [begin, end] = m_bin[data].m_indices[m_bin[data].m_bin_index];
      m_shape[m_nested_dim_index] = end - begin;
      m_data_index[data] = m_bin_stride * begin;
    }
    // else: at end of bins
  }

  void set_bins_index(const scipp::index index) noexcept {
    std::fill(coord_it(0), coord_it(m_inner_ndim), 0);
    if (bin_ndim() == 0 && index != 0) {
      m_coord[m_nested_dim_index] = m_shape[m_nested_dim_index];
    } else {
      extract_indices(index, shape_it(m_inner_ndim), shape_end(),
                      coord_it(m_inner_ndim));
    }

    for (scipp::index data = 0; data < N; ++data) {
      m_bin[data].m_bin_index = flat_index(data, m_inner_ndim, m_ndim);
      load_bin_params(data);
    }
    if (m_shape[m_nested_dim_index] == 0 && !dim_at_end(m_ndim - 1))
      seek_bin();
  }

  void set_to_end_bin() noexcept {
    std::fill(coord_it(), coord_end(), 0);
    const auto last_dim = (bin_ndim() == 0 ? m_nested_dim_index : m_ndim - 1);
    m_coord[last_dim] = m_shape[last_dim];

    for (scipp::index data = 0; data < N; ++data) {
      // Only one dim contributes, all others have coord = 0.
      m_bin[data].m_bin_index = m_coord[last_dim] * m_stride[last_dim][data];
      load_bin_params(data);
    }
  }

  scipp::index flat_index(const scipp::index i_data, scipp::index begin_index,
                          const scipp::index end_index) {
    scipp::index res = 0;
    for (; begin_index < end_index; ++begin_index) {
      res += m_coord[begin_index] * m_stride[begin_index][i_data];
    }
    return res;
  }

  [[nodiscard]] auto coord_it(const scipp::index dim = 0) noexcept {
    return m_coord.begin() + dim;
  }

  [[nodiscard]] auto coord_end() noexcept { return m_coord.begin() + m_ndim; }

  [[nodiscard]] auto shape_it(const scipp::index dim = 0) noexcept {
    return std::next(m_shape.begin(), dim);
  }

  [[nodiscard]] auto shape_end() noexcept { return m_shape.begin() + m_ndim; }

  /// Current flat index into the operands.
  std::array<scipp::index, N> m_data_index = {};
  // This does *not* 0-init the inner arrays!
  /// Stride for each operand in each dimension.
  std::array<std::array<scipp::index, N>, NDIM_MAX> m_stride = {};
  /// Current index in iteration dimensions for both bin and inner dims.
  std::array<scipp::index, NDIM_MAX + 1> m_coord = {};
  /// Shape of the iteration dimensions for both bin and inner dims.
  std::array<scipp::index, NDIM_MAX + 1> m_shape = {};
  /// Total number of dimensions.
  scipp::index m_ndim{0};
  /// Number of dense dimensions, i.e. same as m_ndim when not binned,
  /// else number of dims in bins.
  scipp::index m_inner_ndim{0};
  /// Stride from one bin to the next.
  scipp::index m_bin_stride = {};
  /// Index of dim referred to by bin indices to distinguish, e.g., 2D bins
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

extern template MultiIndex<1>::MultiIndex(const Dimensions &, const Strides &);
extern template MultiIndex<2>::MultiIndex(const Dimensions &, const Strides &,
                                          const Strides &);
extern template MultiIndex<3>::MultiIndex(const Dimensions &, const Strides &,
                                          const Strides &, const Strides &);
extern template MultiIndex<4>::MultiIndex(const Dimensions &, const Strides &,
                                          const Strides &, const Strides &,
                                          const Strides &);

extern template MultiIndex<1>::MultiIndex(binned_tag, const Dimensions &,
                                          const Dimensions &,
                                          const ElementArrayViewParams &);
extern template MultiIndex<2>::MultiIndex(binned_tag, const Dimensions &,
                                          const Dimensions &,
                                          const ElementArrayViewParams &,
                                          const ElementArrayViewParams &);
extern template MultiIndex<3>::MultiIndex(binned_tag, const Dimensions &,
                                          const Dimensions &,
                                          const ElementArrayViewParams &,
                                          const ElementArrayViewParams &,
                                          const ElementArrayViewParams &);
extern template MultiIndex<4>::MultiIndex(binned_tag, const Dimensions &,
                                          const Dimensions &,
                                          const ElementArrayViewParams &,
                                          const ElementArrayViewParams &,
                                          const ElementArrayViewParams &,
                                          const ElementArrayViewParams &);

extern template class MultiIndex<1>;
extern template class MultiIndex<2>;
extern template class MultiIndex<3>;
extern template class MultiIndex<4>;

} // namespace scipp::core
