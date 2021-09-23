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
static void do_accumulate(const std::tuple<Ts...> &types, Op op,
                          const std::string_view &name, Var &&var,
                          const Other &... other) {
  // Bail out (no threading) if:
  // - `other` is implicitly broadcast
  // - `other` are small, to avoid overhead (important for groupby), limit set
  //   by tuning BM_groupby_large_table
  // - reduction to scalar with more than 1 `other`
  constexpr scipp::index small_input = 16384;
  if ((!other.dims().includes(var.dims()) || ...) ||
      ((other.dims().volume() < small_input) && ...) ||
      (sizeof...(other) != 1 && var.dims().ndim() == 0))
    return in_place<false>::transform_data(types, op, name, var, other...);

  const auto reduce_chunk = [&](auto &&out, const Slice slice) {
    // A typical cache line has 64 Byte, which would fit, e.g., 8 doubles. If
    // multiple threads write to different elements in the same cache lines we
    // have "false sharing", with a severe negative performance impact. 128 is a
    // somewhat arbitrary limit at which we can consider it unlikely that two
    // threads would frequently run into falsely shared elements. May be need
    // further tuning.
    const bool avoid_false_sharing = out.dims().volume() < 128;
    auto tmp = avoid_false_sharing ? copy(out) : out;
    [&](const auto &... args) { // force slices to const, avoid readonly issues
      in_place<false>::transform_data(types, op, name, tmp, args...);
    }(other.slice(slice)...);
    if (avoid_false_sharing)
      copy(tmp, out);
  };

  // TODO The parallelism could be improved for cases where the output has more
  // than one dimension, e.g., by flattening the output's dims in all inputs.
  // However, it is nontrivial to detect whether calling `flatten` on `other` is
  // possible without copies so this is not implemented at this point.
  const auto accumulate_parallel = [&]() {
    const auto dim = *var.dims().begin();
    const auto reduce = [&](const auto &range) {
      const Slice slice(dim, range.begin(), range.end());
      reduce_chunk(var.slice(slice), slice);
    };
    const auto size = var.dims()[dim];
    core::parallel::parallel_for(core::parallel::blocked_range(0, size),
                                 reduce);
  };
  if constexpr (sizeof...(other) == 1) {
    const bool reduce_outer =
        (!var.dims().contains(other.dims().labels().front()) || ...);
    // This value is found from benchmarks reducing the outer dimension. Making
    // it larger can improve parallelism further, but increases the overhead
    // from copies. May need further tuning.
    constexpr scipp::index chunking_limit = 65536;
    if (var.dims().ndim() == 0 ||
        (reduce_outer && var.dims()[*var.dims().begin()] < chunking_limit)) {
      // For small output sizes, especially with reduction along the outer
      // dimension, threading via the output's dimension does not provide
      // significant speedup, mainly due to partially transposed memory access
      // patterns. We thus chunk based on the input's dimension, for a 5x
      // speedup in many cases.
      const auto outer_dim = (*other.dims().begin(), ...);
      const auto outer_size = (other.dims()[outer_dim], ...);
      const auto nchunk = std::min(scipp::index(24), outer_size);
      const auto chunk_size = (outer_size + nchunk - 1) / nchunk;
      // The threading approach in used here is possible only under the
      // assumption that op(var, broadcast(var, ...)) leaves var unchanged. This
      // is true for "idempotent" *operations* such as `min` and `max` as well
      // as for the output (initial) *values* used for, e.g., `sum` (zero).
      // However there are situations where *neither* is the case, most notably
      // groupby(...).sum which calls accumulate multiple times *with the same
      // output*.
      // We could still support threading in such cases if the caller can
      // provide an initial value to use for initializing the output buffer
      // (instead of a broadcast of the output). For now we simply bail out if
      // we detect non-idempotent initial values.
      auto v = copy(var);
      if (in_place<false>::transform_data(types, op, name, v, var); var != v)
        return in_place<false>::transform_data(types, op, name, var, other...);
      v = copy(
          broadcast(var, merge({Dim::InternalAccumulate, nchunk}, var.dims())));
      const auto reduce = [&](const auto &range) {
        for (scipp::index i = range.begin(); i < range.end(); ++i) {
          const Slice slice(outer_dim, std::min(i * chunk_size, outer_size),
                            std::min((i + 1) * chunk_size, outer_size));
          reduce_chunk(v.slice({Dim::InternalAccumulate, i}), slice);
        }
      };
      core::parallel::parallel_for(core::parallel::blocked_range(0, nchunk, 1),
                                   reduce);
      in_place<false>::transform_data(types, op, name, var, v);
    } else {
      accumulate_parallel();
    }
  } else {
    accumulate_parallel();
  }
}

template <class... Ts, class Op, class Var, class... Other>
static void accumulate(const std::tuple<Ts...> &types, Op op,
                       const std::string_view name, Var &&var,
                       Other &&... other) {
  // `other` not const, threading for cumulative ops not possible
  if constexpr ((!std::is_const_v<std::remove_reference_t<Other>> || ...))
    return in_place<false>::transform_data(types, op, name, var, other...);
  else
    do_accumulate(types, op, name, std::forward<Var>(var), other...);
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
                         const std::string_view name) {
  // Note lack of dims check here and below: transform_data calls `merge` on the
  // dims which does the required checks, supporting broadcasting of outputs and
  // inputs but ensuring compatibility otherwise.
  detail::accumulate(type_tuples<Ts...>(op), op, name, std::forward<Var>(var),
                     other);
}

template <class... Ts, class Var, class Op>
void accumulate_in_place(Var &&var, const Variable &var1, const Variable &var2,
                         Op op, const std::string_view name) {
  detail::accumulate(type_tuples<Ts...>(op), op, name, std::forward<Var>(var),
                     var1, var2);
}

template <class... Ts, class Var, class Op>
void accumulate_in_place(Var &&var, Variable &var1, const Variable &var2,
                         const Variable &var3, Op op,
                         const std::string_view name) {
  detail::accumulate(type_tuples<Ts...>(op), op, name, std::forward<Var>(var),
                     var1, var2, var3);
}

} // namespace scipp::variable
