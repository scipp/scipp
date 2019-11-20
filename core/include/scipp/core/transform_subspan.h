// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_TRANSFORM_SUBSPAN_H
#define SCIPP_CORE_TRANSFORM_SUBSPAN_H

#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

namespace scipp::core {

/// Non-element-wise transform.
///
/// This is a specialized version of transform, handling the case of inputs (and
/// output) that differ along one of their dimensions. Applications are mixing
/// of sparse and dense data, as well as operations that change the length of a
/// dimension (such as rebin). The syntax for the user-provided operator is
/// special and differs from that of `transform` and `transform_in_place`, and
/// also the tuple of supported type combinations is special:
/// 1. The overload for the transform of the unit is as for `transform`, i.e.,
///    returns the new unit.
/// 2. The overload handling the data has one extra argument. This additional
///    (first) argument is the "out" argument, as used in `transform_in_place`.
/// 3. The tuple of supported type combinations must include the type of the out
///    argument as the first type in the inner tuples. Currently all supported
///    combinations must have the same output type.
/// 4. The output type and the type of non-sparse inputs that depend on `dim`
///    must be specified as `span<T>`. The user-provided lambda is called with a
///    span of values for these arguments.
/// 5. Use the flag transform_flags::expect_variance_arg<0> to control whether
///    the output should have variances or not.
template <class Types, class Op>
[[nodiscard]] Variable transform_subspan(const Dim dim, const scipp::index size,
                                         VariableConstProxy var1,
                                         VariableConstProxy var2, Op op) {
  auto dims1 = var1.dims();
  auto dims2 = var2.dims();
  dims1.erase(dim);
  dims2.erase(dim);
  auto dims = merge(dims1, dims2);
  dims.addInner(dim, size);

  // This will cause failure below unless the output type (first type in inner
  // tuple) is the same in all inner tuples.
  using OutT =
      typename std::tuple_element_t<0,
                                    std::tuple_element_t<0, Types>>::value_type;
  Variable out =
      std::is_base_of_v<transform_flags::expect_variance_arg_t<0>, Op>
          ? makeVariableWithVariances<OutT>(dims)
          : makeVariable<OutT>(dims);

  Variable var1_subspans;
  Variable var2_subspans;
  if (!var1.dims().sparse()) {
    var1_subspans = subspan_view(var1, dim);
    var1 = VariableConstProxy(var1_subspans);
  }
  if (!var2.dims().sparse()) {
    var2_subspans = subspan_view(var2, dim);
    var2 = VariableConstProxy(var2_subspans);
  }

  out.setUnit(op(var1.unit(), var2.unit()));
  in_place<false>::transform_data(Types{}, op, subspan_view(out, dim), var1,
                                  var2);
  return out;
}

} // namespace scipp::core

#endif // SCIPP_CORE_TRANSFORM_SUBSPAN_H
