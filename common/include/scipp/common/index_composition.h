// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include <array>
#include <cassert>

#include "scipp/common/index.h"

namespace scipp {
/// Compute a flat index from strides and a multi-dimensional index.
///
/// @return sum_{i=0}^{ndim} ( strides[i] * indices[i] )
/// @note This function uses *strides* and not a shape, meaning strides[d]
///       is not the extent of the array in dimension d but rather the step
///       length in the flat index to advance one element in d.
///       It is thus *not* complementary to extract_indices.
template <size_t Ndim_max>
constexpr scipp::index
flat_index_from_strides(const std::array<scipp::index, Ndim_max> &strides,
                        const std::array<scipp::index, Ndim_max> &indices,
                        const scipp::index ndim) noexcept {
  assert(ndim <= Ndim_max);
  auto strides_it = strides.begin();
  auto indices_it = indices.begin();
  const auto strides_end = std::next(strides_it, ndim);
  scipp::index result = 0;
  for (; strides_it != strides_end; ++strides_it, ++indices_it) {
    result += *strides_it * *indices_it;
  }
  return result;
}

/// Extract individual indices from a flat index.
///
/// Let
///     I = i_0 + l_0 * (i_1 + l_1 * (i_2 + ... (i_{n-2} + l_{n-2} * i_{n-1})))
/// be a flat index computed from indices {i_d} and shape {l_d} in
/// 'column-major' order. Here, this means that i_0 is the fasted moving index
/// and i_{n-1} is slowest.
///
/// @param flat_index I
/// @param ndim n
/// @param shape {l_d}
/// @param indices {i_d}
/// @note This function uses a *shape*, i.e. individual dimension sizes
///       to encode the size of the array.
///       It is thus *not* complementary to flat_index_from_strides.
template <size_t Ndim_max>
constexpr void
extract_indices(scipp::index flat_index, const scipp::index ndim,
                const std::array<scipp::index, Ndim_max> &shape,
                std::array<scipp::index, Ndim_max> &indices) noexcept {
  assert(ndim <= Ndim_max);
  for (scipp::index dim = 0; dim < ndim; ++dim) {
    const scipp::index aux = flat_index / shape[dim];
    indices[dim] = flat_index - aux * shape[dim];
    flat_index = aux;
  }
}
/* Implementation notes for extract_indices
 *
 * With ndim == 2, we have
 *     I = i_0 + l_0 * i_1
 * All numbers are positive integers. Thus I can be decomposed using
 * integer division as follows:
 *     x = I / l_0
 *     i_0 = I - x * l_0
 *     i_1 = x
 *
 * With ndim == 3, we have
 *     I = i_0 + l_0 * (i_1 + l_1 * i_2)
 * which can be decomposed as above:
 *     x = I / l_0
 *     i_0 = I - x * l_0
 * Noting that
 *     x = i_1 _ l_1 * i_2
 * we can compute i_1 and i_2 by applying the algorithm recursively.
 *
 * The function implements this algorithm for arbitrary dimensions by rolling
 * the recursion into a loop.
 */

} // namespace scipp