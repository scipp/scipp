// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/multi_index.h"
#include "scipp/core/except.h"

namespace scipp::core {

namespace detail {
void validate_bin_indices_impl(const ElementArrayViewParams &param0,
                               const ElementArrayViewParams &param1) {
  const auto iterDims = param0.dims();
  auto index = MultiIndex(iterDims, param0.strides(), param1.strides());
  const auto indices0 = param0.bucketParams().indices;
  const auto indices1 = param1.bucketParams().indices;
  constexpr auto size = [](const auto range) {
    return range.second - range.first;
  };
  for (scipp::index i = 0; i < iterDims.volume(); ++i) {
    const auto [i0, i1] = index.get();
    if (size(indices0[i0]) != size(indices1[i1]))
      throw except::BinnedDataError(
          "Bin size mismatch in operation with binned data. Refer to "
          "https://scipp.github.io/user-guide/binned-data/"
          "computation.html#Overview-and-Quick-Reference for equivalent "
          "operations for binned data (event data).");
    index.increment();
  }
}

template <size_t... I, class... StridesArgs>
bool can_be_flattened(const scipp::index dim, const scipp::index size,
                      std::index_sequence<I...>,
                      std::array<scipp::index, sizeof...(I)> &rewind,
                      const StridesArgs &... strides) {
  const bool res = ((strides[dim] == rewind[I] && strides[dim] != 0) && ...);
  ((rewind[I] = size * strides[dim]), ...);
  return res;
}

// non_flattenable_dim is in the storage order of Dimensions & Strides.
// It is not possible to flatten dimensions outside of the bin-slice dim
// because they are sliced by that dim and their layout changes depending on
// the current bin.
// But the inner dimensions always have the same layout.
template <class... StridesArgs>
[[nodiscard]] scipp::index
flatten_dims(const scipp::span<std::array<scipp::index, sizeof...(StridesArgs)>>
                 &out_strides,
             scipp::index *const out_shape, const Dimensions &dims,
             const scipp::index non_flattenable_dim,
             const StridesArgs &... strides) {
  constexpr scipp::index N = sizeof...(StridesArgs);
  std::array strides_array{std::ref(strides)...};
  std::array<scipp::index, N> rewind{};
  scipp::index dim_write = 0;
  for (scipp::index dim_read = dims.ndim() - 1; dim_read >= 0; --dim_read) {
    const auto size = dims.size(dim_read);
    if (dim_read > non_flattenable_dim &&
        detail::can_be_flattened(dim_read, size, std::make_index_sequence<N>{},
                                 rewind, strides...)) {
      out_shape[dim_write - 1] *= size;
    } else {
      out_shape[dim_write] = size;
      for (scipp::index data = 0; data < N; ++data) {
        out_strides[dim_write][data] = strides_array[data].get()[dim_read];
      }
      ++dim_write;
    }
  }
  return dim_write;
}
} // namespace detail

template <scipp::index N>
template <class... StridesArgs>
MultiIndex<N>::MultiIndex(const Dimensions &iter_dims,
                          const StridesArgs &... strides) {
  const size_t shape_buffer_size =
      detail::get_shape_buffer_size(iter_dims.ndim());
  const size_t strides_buffer_size =
      detail::get_strides_buffer_size<N>(iter_dims.ndim());

  const auto temp_buffer =
      std::make_unique<scipp::index[]>(strides_buffer_size + shape_buffer_size);
  scipp::index *const shape_buffer = temp_buffer.get() + strides_buffer_size;
  m_ndim = detail::flatten_dims(scipp::span{m_stride.begin(), NDIM_MAX},
                                shape_buffer, iter_dims, 0, strides...);
  m_inner_ndim = m_ndim;

  m_buffer =
      std::make_unique<scipp::index[]>(detail::get_buffer_size<N>(m_ndim));
  std::copy(shape_buffer, shape_buffer + m_ndim, shape_it());
}

template <scipp::index N>
template <class... Params>
MultiIndex<N>::MultiIndex(binned_tag, const Dimensions &inner_dims,
                          const Dimensions &bin_dims, const Params &... params)
    : m_bin{BinIterator(params)...} {
  detail::validate_bin_indices(params...);

  const Dim slice_dim = detail::get_slice_dim(params.bucketParams()...);

  const size_t inner_shape_buffer_size =
      detail::get_shape_buffer_size(inner_dims.ndim());
  const size_t inner_strides_buffer_size =
      detail::get_strides_buffer_size<N>(inner_dims.ndim());
  const size_t bin_shape_buffer_size =
      detail::get_shape_buffer_size(bin_dims.ndim());
  const size_t bin_strides_buffer_size =
      detail::get_strides_buffer_size<N>(bin_dims.ndim());

  const auto temp_buffer = std::make_unique<scipp::index[]>(
      inner_strides_buffer_size + inner_shape_buffer_size +
      bin_shape_buffer_size + bin_strides_buffer_size);
  scipp::index *const inner_strides_buffer = temp_buffer.get();
  scipp::index *const inner_shape_buffer =
      inner_strides_buffer + inner_strides_buffer_size;
  scipp::index *const bin_strides_buffer =
      inner_shape_buffer + inner_shape_buffer_size;
  scipp::index *const bin_shape_buffer =
      bin_strides_buffer + bin_strides_buffer_size;

  m_inner_ndim = detail::flatten_dims(
      scipp::span{m_stride.begin(), NDIM_MAX}, inner_shape_buffer, inner_dims,
      inner_dims.index(slice_dim),
      params.bucketParams() ? Strides{inner_dims} : Strides{}...);
  m_ndim = m_inner_ndim +
           detail::flatten_dims(
               scipp::span{m_stride.begin() + m_inner_ndim,
                           static_cast<size_t>(NDIM_MAX - m_inner_ndim)},
               bin_shape_buffer, bin_dims, 0, params.strides()...);

  m_buffer =
      std::make_unique<scipp::index[]>(detail::get_buffer_size<N>(m_ndim));
  // using manual size here because the max in get_* would overflow the buffer
  std::copy(inner_shape_buffer, inner_shape_buffer + m_inner_ndim, shape_it());
  std::copy(bin_shape_buffer, bin_shape_buffer + (m_ndim - m_inner_ndim),
            shape_it(m_inner_ndim));

  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  m_bin_stride = inner_dims.offset(slice_dim);
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  m_nested_dim_index = m_inner_ndim - inner_dims.index(slice_dim) - 1;

  if (bin_dims.volume() == 0) {
    return; // Operands are empty, there are no bins to load.
  }
  for (scipp::index data = 0; data < N; ++data) {
    load_bin_params(data);
  }
  if (shape(m_nested_dim_index) == 0)
    seek_bin();
}

template MultiIndex<1>::MultiIndex(const Dimensions &, const Strides &);
template MultiIndex<2>::MultiIndex(const Dimensions &, const Strides &,
                                   const Strides &);
template MultiIndex<3>::MultiIndex(const Dimensions &, const Strides &,
                                   const Strides &, const Strides &);
template MultiIndex<4>::MultiIndex(const Dimensions &, const Strides &,
                                   const Strides &, const Strides &,
                                   const Strides &);

template MultiIndex<1>::MultiIndex(binned_tag, const Dimensions &,
                                   const Dimensions &,
                                   const ElementArrayViewParams &);
template MultiIndex<2>::MultiIndex(binned_tag, const Dimensions &,
                                   const Dimensions &,
                                   const ElementArrayViewParams &,
                                   const ElementArrayViewParams &);
template MultiIndex<3>::MultiIndex(binned_tag, const Dimensions &,
                                   const Dimensions &,
                                   const ElementArrayViewParams &,
                                   const ElementArrayViewParams &,
                                   const ElementArrayViewParams &);
template MultiIndex<4>::MultiIndex(binned_tag, const Dimensions &,
                                   const Dimensions &,
                                   const ElementArrayViewParams &,
                                   const ElementArrayViewParams &,
                                   const ElementArrayViewParams &,
                                   const ElementArrayViewParams &);

template class MultiIndex<1>;
template class MultiIndex<2>;
template class MultiIndex<3>;
template class MultiIndex<4>;
template class MultiIndex<5>;
template class MultiIndex<6>;

} // namespace scipp::core
