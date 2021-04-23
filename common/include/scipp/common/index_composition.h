// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include <array>
#include <cassert>
#include <cstddef>

#include "scipp/common/index.h"

namespace scipp {
/// Compute a flat index from strides and a multi-dimensional index.
///
/// @return sum_{i=0}^{ndim} ( strides[i] * indices[i] )
/// @note This function uses *strides* and not a shape, meaning strides[d]
///       is not the extent of the array in dimension d but rather the step
///       length in the flat index to advance one element in d.
///       It is thus *not* complementary to extract_indices.
template <class ForwardIt1, class ForwardIt2>
constexpr auto flat_index_from_strides(ForwardIt1 strides_it,
                                       ForwardIt2 strides_end,
                                       ForwardIt2 indices_it) noexcept {
  std::remove_const_t<std::remove_reference_t<decltype(*strides_it)>> result =
      0;
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
/// If I == prod_{d=0}^ndim (l_d), i.e. one element past the end,
/// the resulting indices are i_d = 0 for d < ndim-1, i_{ndim-1} = l_{ndim-1}
/// unless l_{ndim-1} = 0, see below.
/// This allows setting 'end-iterators' in a well defined manner.
/// However, the result is undefined for greater values of I.
///
/// Values of array elements in `indices` with d > ndim-1 are unspecified.
///
/// Any number of l_d maybe 0 which yields i_d = 0.
/// Except for the one-past-the-end case described above, i_{ndim-1} = 1 if
/// l_{ndim-1} to allow this case to be distinguishable from an index
/// to the end.
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
  assert(ndim <= static_cast<scipp::index>(Ndim_max));
  for (scipp::index dim = 0; dim < ndim - 1; ++dim) {
    if (shape[dim] != 0) {
      const scipp::index aux = flat_index / shape[dim];
      indices[dim] = flat_index - aux * shape[dim];
      flat_index = aux;
    } else {
      indices[dim] = 0;
    }
  }
  if (ndim > 0) {
    indices[ndim - 1] = flat_index;
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