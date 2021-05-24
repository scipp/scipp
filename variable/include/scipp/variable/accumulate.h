// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file Accumulation functions for variables, based on transform.
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/shape.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

namespace detail {
template <class... Ts, class Op, class Var, class... Other>
static void accumulate(const std::tuple<Ts...> &types, Op op,
                       const std::string_view &name, Var &&var,
                       Other &&... other) {
  // Cumulative operations may write to `other`, threading not possible.
  if constexpr ((!std::is_const_v<std::remove_reference_t<Other>> || ...))
    return in_place<false>::transform_data(types, op, name, var, other...);
  // Bail out if `other` is implicitly broadcast
  if ((!other.dims().includes(var.dims()) || ...))
    return in_place<false>::transform_data(types, op, name, var, other...);

  const auto reduce_chunk = [&](auto &&out, const Slice slice) {
    const bool avoid_false_sharing = out.dims().volume() < 128;
    auto tmp = avoid_false_sharing ? copy(out) : out;
    in_place<false>::transform_data(types, op, name, tmp,
                                    other.slice(slice)...);
    if (avoid_false_sharing)
      copy(tmp, out);
  };

  // TODO The parallelism could be improved for cases where the output has more
  // than one dimension, e.g., by flattening the output's dims in all inputs.
  // However, it is nontrivial to detect whether calling `flatten` on `other` is
  // possible without copies so this is not implemented at this point.
  const bool reduce_outer =
      (!var.dims().contains(other.dims().labels().front()) || ...);
  if (var.dims().ndim() == 0 ||
      (reduce_outer && var.dims()[*var.dims().begin()] < 65536)) {
    // For small output sizes, especially with reduction along the outer
    // dimension, threading via the output's dimension does not provide
    // significant speedup, mainly due to partially transposed memory access
    // patterns. We thus chunk based on the input's dimension, for a 5x speedup
    // in many cases.
    const auto outer_dim = (*other.dims().begin(), ...);
    const auto outer_size = (other.dims()[outer_dim], ...);
    const auto nchunk = std::min(scipp::index(24), outer_size);
    const auto chunk_size = (outer_size + nchunk - 1) / nchunk;
    auto v = copy(broadcast(var, merge({Dim::Internal0, nchunk}, var.dims())));
    const auto reduce = [&](const auto &range) {
      for (scipp::index i = range.begin(); i < range.end(); ++i) {
        const Slice slice(outer_dim, i * chunk_size,
                          std::min((i + 1) * chunk_size, outer_size));
        reduce_chunk(v.slice({Dim::Internal0, i}), slice);
      }
    };
    core::parallel::parallel_for(core::parallel::blocked_range(0, nchunk, 1),
                                 reduce);
    in_place<false>::transform_data(types, op, name, var, v);
  } else {
    const auto dim = *var.dims().begin();
    const auto reduce = [&](const auto &range) {
      const Slice slice(dim, range.begin(), range.end());
      reduce_chunk(var.slice(slice), slice);
    };
    const auto size = var.dims()[dim];
    core::parallel::parallel_for(core::parallel::blocked_range(0, size),
                                 reduce);
  }
}
} // namespace detail

/// Accumulate data elements of a variable in-place.
///
/// This is equivalent to `transform_in_place`, with the only difference that
/// the dimension check of the inputs is reversed. That is, it must be possible
/// to broadcast the dimension of the first argument to that of the other
/// argument. As a consequence, the operation may be applied multiple times to
/// the same output element, effectively accumulating the result.
///
/// WARNING: In contrast to the transform algorithms, accumulate does not touch
/// the unit, since it would be hard to track, e.g., in multiplication
/// operations.
template <class... Ts, class Var, class Other, class Op>
void accumulate_in_place(Var &&var, Other &&other, Op op,
                         const std::string_view &name = "operation") {
  // Note lack of dims check here and below: transform_data calls `merge` on the
  // dims which does the required checks, supporting broadcasting of outputs and
  // inputs but ensuring compatibility otherwise.
  detail::accumulate(type_tuples<Ts...>(op), op, name, std::forward<Var>(var),
                     other);
}

template <class... Ts, class Var, class Op>
void accumulate_in_place(Var &&var, const Variable &var1, const Variable &var2,
                         Op op, const std::string_view &name = "operation") {
  in_place<false>::transform_data(type_tuples<Ts...>(op), op, name,
                                  std::forward<Var>(var), var1, var2);
}

template <class... Ts, class Var, class Op>
void accumulate_in_place(Var &&var, Variable &var1, const Variable &var2,
                         const Variable &var3, Op op,
                         const std::string_view &name = "operation") {
  in_place<false>::transform_data(type_tuples<Ts...>(op), op, name,
                                  std::forward<Var>(var), var1, var2, var3);
}

} // namespace scipp::variable
