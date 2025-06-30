// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/core/multi_index.h"
#include "scipp-core_export.h"
#include "scipp/core/except.h"

namespace scipp::core {

template class SCIPP_CORE_EXPORT MultiIndex<1>;
template class SCIPP_CORE_EXPORT MultiIndex<2>;
template class SCIPP_CORE_EXPORT MultiIndex<3>;
template class SCIPP_CORE_EXPORT MultiIndex<4>;
template class SCIPP_CORE_EXPORT MultiIndex<5>;

namespace {
void validate_bin_indices_impl(const ElementArrayViewParams &param0,
                               const ElementArrayViewParams &param1) {
  const auto &iterDims = param0.dims();
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

template <class Param> void validate_bin_indices(const Param &) {}

/// Check that corresponding bins have matching sizes.
template <class Param0, class Param1, class... Params>
void validate_bin_indices(const Param0 &param0, const Param1 &param1,
                          const Params &...params) {
  if (param0.bucketParams() && param1.bucketParams())
    validate_bin_indices_impl(param0, param1);
  if (param0.bucketParams())
    validate_bin_indices(param0, params...);
  else
    validate_bin_indices(param1, params...);
}

inline auto get_slice_dim() { return Dim::Invalid; }

template <class T, class... Ts>
auto get_slice_dim(const T &param, const Ts &...params) {
  return param ? param.dim : get_slice_dim(params...);
}

template <class T>
[[nodiscard]] auto make_span(T &&array, const scipp::index begin) {
  return std::span{array.data() + begin,
                   static_cast<size_t>(NDIM_OP_MAX - begin)};
}

template <class StridesArg>
[[nodiscard]] scipp::index value_or_default(const StridesArg &strides,
                                            const scipp::index i) {
  return i < strides.size() ? strides[i] : 0;
}

template <size_t... I, class... StridesArgs>
bool can_be_flattened(
    const scipp::index dim, const scipp::index size, std::index_sequence<I...>,
    std::array<scipp::index, sizeof...(I)> &strides_for_contiguous,
    const StridesArgs &...strides) {
  const bool res =
      ((value_or_default(strides, dim) == strides_for_contiguous[I]) && ...);
  ((strides_for_contiguous[I] = size * value_or_default(strides, dim)), ...);
  return res;
}

// non_flattenable_dim is in the storage order of Dimensions & Strides.
// It is not possible to flatten dimensions outside of the bin-slice dim
// because they are sliced by that dim and their layout changes depending on
// the current bin.
// But the inner dimensions always have the same layout.
template <class... StridesArgs>
[[nodiscard]] scipp::index
flatten_dims(const std::span<std::array<scipp::index, sizeof...(StridesArgs)>>
                 &out_strides,
             const std::span<scipp::index> &out_shape, const Dimensions &dims,
             const scipp::index non_flattenable_dim,
             const StridesArgs &...strides) {
  constexpr scipp::index N = sizeof...(StridesArgs);
  std::array strides_array{std::ref(strides)...};
  std::array<scipp::index, N> strides_for_contiguous{};
  scipp::index dim_write = 0;
  for (scipp::index dim_read = dims.ndim() - 1; dim_read >= 0; --dim_read) {
    if (dim_write >= static_cast<scipp::index>(out_shape.size()))
      throw std::runtime_error("Operations with more than " +
                               std::to_string(NDIM_OP_MAX) +
                               " dimensions are not supported. "
                               "For binned data, the combined bin+event "
                               "dimensions count");
    const auto size = dims.size(dim_read);
    if (dim_read > non_flattenable_dim &&
        dim_write > 0 && // need to write at least one inner dim
        can_be_flattened(dim_read, size, std::make_index_sequence<N>{},
                         strides_for_contiguous, strides...)) {
      out_shape[dim_write - 1] *= size;
    } else {
      out_shape[dim_write] = size;
      for (scipp::index data = 0; data < N; ++data) {
        out_strides[dim_write][data] =
            value_or_default(strides_array[data].get(), dim_read);
      }
      ++dim_write;
    }
  }
  return dim_write;
}
} // namespace

template <scipp::index N>
template <class... StridesArgs>
MultiIndex<N>::MultiIndex(const Dimensions &iter_dims,
                          const StridesArgs &...strides)
    : m_ndim{flatten_dims(make_span(m_stride, 0), make_span(m_shape, 0),
                          iter_dims, 0, strides...)},
      m_inner_ndim{m_ndim} {}

template <scipp::index N>
template <class... Params>
MultiIndex<N>::MultiIndex(binned_tag, const Dimensions &inner_dims,
                          const Dimensions &bin_dims, const Params &...params)
    : m_bin{BinIterator(params.bucketParams(), bin_dims.volume())...} {
  validate_bin_indices(params...);

  const Dim slice_dim = get_slice_dim(params.bucketParams()...);

  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  m_inner_ndim = flatten_dims(
      make_span(m_stride, 0), make_span(m_shape, 0), inner_dims,
      inner_dims.index(slice_dim),
      params.bucketParams() ? params.bucketParams().strides : Strides{}...);
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  m_ndim = m_inner_ndim + flatten_dims(make_span(m_stride, m_inner_ndim),
                                       make_span(m_shape, m_inner_ndim),
                                       bin_dims, 0, params.strides()...);

  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  m_nested_dim_index = m_inner_ndim - inner_dims.index(slice_dim) - 1;

  for (scipp::index data = 0; data < N; ++data) {
    load_bin_params(data);
  }
  if (m_shape[m_nested_dim_index] == 0 || bin_dims.volume() == 0)
    seek_bin();
}

template SCIPP_CORE_EXPORT MultiIndex<1>::MultiIndex(const Dimensions &,
                                                     const Strides &);
template SCIPP_CORE_EXPORT
MultiIndex<2>::MultiIndex(const Dimensions &, const Strides &, const Strides &);
template SCIPP_CORE_EXPORT MultiIndex<3>::MultiIndex(const Dimensions &,
                                                     const Strides &,
                                                     const Strides &,
                                                     const Strides &);
template SCIPP_CORE_EXPORT
MultiIndex<4>::MultiIndex(const Dimensions &, const Strides &, const Strides &,
                          const Strides &, const Strides &);
template SCIPP_CORE_EXPORT
MultiIndex<5>::MultiIndex(const Dimensions &, const Strides &, const Strides &,
                          const Strides &, const Strides &, const Strides &);

template SCIPP_CORE_EXPORT
MultiIndex<1>::MultiIndex(binned_tag, const Dimensions &, const Dimensions &,
                          const ElementArrayViewParams &);
template SCIPP_CORE_EXPORT
MultiIndex<2>::MultiIndex(binned_tag, const Dimensions &, const Dimensions &,
                          const ElementArrayViewParams &,
                          const ElementArrayViewParams &);
template SCIPP_CORE_EXPORT
MultiIndex<3>::MultiIndex(binned_tag, const Dimensions &, const Dimensions &,
                          const ElementArrayViewParams &,
                          const ElementArrayViewParams &,
                          const ElementArrayViewParams &);
template SCIPP_CORE_EXPORT MultiIndex<4>::MultiIndex(
    binned_tag, const Dimensions &, const Dimensions &,
    const ElementArrayViewParams &, const ElementArrayViewParams &,
    const ElementArrayViewParams &, const ElementArrayViewParams &);
template SCIPP_CORE_EXPORT MultiIndex<5>::MultiIndex(
    binned_tag, const Dimensions &, const Dimensions &,
    const ElementArrayViewParams &, const ElementArrayViewParams &,
    const ElementArrayViewParams &, const ElementArrayViewParams &,
    const ElementArrayViewParams &);

} // namespace scipp::core
