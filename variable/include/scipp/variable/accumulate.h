// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file Accumulation functions for variables, based on transform.
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/transform.h"

namespace scipp::variable {

namespace detail {
template <class... Ts, class Op, class Var, class... Other>
static void accumulate(const std::tuple<Ts...> &types, Op op,
                       const std::string_view &name, Var &&var,
                       Other &&... other) {
  if (var.dims().ndim() == 0 || (!other.dims().includes(var.dims()) || ...)) {
    // Bail out if output is scalar or input is broadcast => no multi-threading.
    // TODO Could implement multi-threading for scalars by broadcasting the
    // output before slicing, but this will require extra care since there are
    // cases (specifically cumulative operations) where also the secodn argument
    // is being written two, in which case broadcasting must not be done.
    return in_place<false>::transform_data(types, op, name, var, other...);
  }
  // TODO The parallelism could be improved for cases where the output has more
  // than one dimension, e.g., by flattening the output's dims in all inputs.
  // However, it is nontrivial to detect whether calling `flatten` on `other` is
  // possible without copies so this is not implemented at this point.
  const auto dim = *var.dims().begin();
  const auto reduce = [&](const auto &range) {
    const Slice slice(dim, range.begin(), range.end());
    auto out = var.slice(slice);
    const bool avoid_false_sharing = range.end() - range.begin() < 128;
    if (avoid_false_sharing)
      out = copy(out);
    in_place<false>::transform_data(types, op, name, out,
                                    other.slice(slice)...);
    if (avoid_false_sharing)
      copy(out, var.slice(slice));
  };
  const bool reduce_outer =
      (!var.dims().contains(other.dims().labels().front()) || ...);
  // Avoid slow transposed reads when accumulating along outer dim
  // TODO Even with this grain size we see essentially no multi-threaded speedup
  // for accumulation along the *outer* dim if there are less than about 500
  // output elements. The solution could be to chunk along the input dim, but
  // this possible only if exclusively `var` is modified by `op`.
  const auto grainsize =
      reduce_outer ? std::max(scipp::index(32), var.dims()[dim] / 24) : -1;
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, var.dims()[dim], grainsize), reduce);
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
