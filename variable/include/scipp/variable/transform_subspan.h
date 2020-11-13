// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

namespace transform_subspan_detail {
static constexpr auto erase = [](Dimensions dims, const Dim dim) {
  dims.erase(dim);
  return dims;
};

static constexpr auto maybe_subspan = [](VariableConstView &var,
                                         const Dim dim) {
  auto ret = std::make_unique<Variable>();
  // There is a special case handled here when the client passes a `var`
  // containing spans: We do not create span<span<T>>. The effect of this is
  // that such a `var` may depend on `dim`. This implies that the transform will
  // actually perform an accumulation, since the out arg is passed as a subpsna
  // over `dim`, i.e., does not depend on `dim`. Make sure to request zero init
  // of the output in that case, and do not init data in `op`.
  if (var.dims().contains(dim) && !core::is_span(var.dtype())) {
    *ret = subspan_view(var, dim);
    var = *ret;
  }
  return ret;
};
} // namespace transform_subspan_detail

template <class... Types, class Op, class... Var>
[[nodiscard]] Variable transform_subspan_impl(const DType type, const Dim dim,
                                              const scipp::index size, Op op,
                                              Var... var) {
  using namespace transform_subspan_detail;

  auto dims =
      merge(var.dims().contains(dim) ? erase(var.dims(), dim) : var.dims()...);
  dims.addInner(dim, size);
  const bool variance =
      std::is_base_of_v<core::transform_flags::expect_variance_arg_t<0>, Op> ||
      (std::is_base_of_v<
           core::transform_flags::expect_in_variance_if_out_variance_t, Op> &&
       (var.hasVariances() || ...));
  Variable out =
      variableFactory().create(type, dims, op(var.unit()...), variance);
  if constexpr (std::is_base_of_v<core::transform_flags::zero_output_t, Op>)
    fill(out, 0.0 * op(var.unit()...));

  const auto keep_subspan_vars_alive = std::array{maybe_subspan(var, dim)...};

  in_place<false>::transform_data(type_tuples<Types...>(op), op,
                                  subspan_view(out, dim), var...);
  return out;
}

/// Non-element-wise transform.
///
/// This is a specialized version of transform, handling the case of inputs (and
/// output) that differ along one of their dimensions. Applications are mixing
/// of events and dense data, as well as operations that change the length of a
/// dimension (such as rebin). The syntax for the user-provided operator is
/// special and differs from that of `transform` and `transform_in_place`, and
/// also the tuple of supported type combinations is special:
/// 1. The overload for the transform of the unit is as for `transform`, i.e.,
///    returns the new unit.
/// 2. The overload handling the data has one extra argument. This additional
///    (first) argument is the "out" argument, as used in `transform_in_place`.
/// 3. The tuple of supported type combinations must include the type of the out
///    argument as the first type in the inner tuples. The output type must be
///    passed at runtime as the first argument. `transform_subspan` DOES NOT
///    INITIALIZE the output array, i.e., `Op` must take care of initializating
///    the respective subspans. This is done for improved performance, avoiding
///    streaming/writing to memory twice. Optionally, initialization can be
///    requested by passing an operator inheriting
///    core::transform_flags::zero_output_t.
/// 4. The output type and the type of non-events inputs that depend on `dim`
///    must be specified as `span<T>`. The user-provided lambda is called with a
///    span of values for these arguments.
/// 5. Use the flag transform_flags::expect_variance_arg<0> to control whether
///    the output should have variances or not.
template <class... Types, class Op>
[[nodiscard]] Variable transform_subspan(const DType type, const Dim dim,
                                         const scipp::index size,
                                         const VariableConstView &var1,
                                         const VariableConstView &var2, Op op) {
  return transform_subspan_impl<Types...>(type, dim, size, op, var1, var2);
}

template <class... Types, class Op>
[[nodiscard]] Variable
transform_subspan(const DType type, const Dim dim, const scipp::index size,
                  const VariableConstView &var1, const VariableConstView &var2,
                  const VariableConstView &var3, Op op) {
  return transform_subspan_impl<Types...>(type, dim, size, op, var1, var2,
                                          var3);
}

} // namespace scipp::variable
