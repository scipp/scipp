// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

namespace transform_subspan_detail {
static constexpr auto erase = [](Dimensions dims, const Dim dim) {
  dims.erase(dim);
  return dims;
};

static constexpr auto maybe_subspan = [](const Variable &var, const Dim dim) {
  if (var.dims().contains(dim))
    return subspan_view(var, dim);
  return var;
};
} // namespace transform_subspan_detail

template <class... Types, class Op, class... Var>
[[nodiscard]] Variable
transform_subspan_impl(const DType type, const Dim dim, const scipp::index size,
                       Op op, const std::string_view &name, Var... var) {
  using namespace transform_subspan_detail;

  auto dims =
      merge(var.dims().contains(dim) ? erase(var.dims(), dim) : var.dims()...);
  dims.addInner(dim, size);
  const bool variance =
      std::is_base_of_v<core::transform_flags::expect_variance_arg_t<0>, Op> ||
      (std::is_base_of_v<
           core::transform_flags::expect_in_variance_if_out_variance_t, Op> &&
       (var.has_variances() || ...));
  Variable out =
      variableFactory().create(type, dims, op(var.unit()...), variance);

  in_place<false>::transform_data(type_tuples<Types...>(op), op, name,
                                  subspan_view(out, dim),
                                  maybe_subspan(var, dim)...);
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
[[nodiscard]] Variable
transform_subspan(const DType type, const Dim dim, const scipp::index size,
                  const Variable &var1, const Variable &var2, Op op,
                  const std::string_view &name = "operation") {
  return transform_subspan_impl<Types...>(type, dim, size, op, name, var1,
                                          var2);
}

template <class... Types, class Op>
[[nodiscard]] Variable
transform_subspan(const DType type, const Dim dim, const scipp::index size,
                  const Variable &var1, const Variable &var2,
                  const Variable &var3, Op op,
                  const std::string_view &name = "operation") {
  return transform_subspan_impl<Types...>(type, dim, size, op, name, var1, var2,
                                          var3);
}

} // namespace scipp::variable
