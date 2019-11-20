// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_TRANSFORM_RESIZE_H
#define SCIPP_CORE_TRANSFORM_RESIZE_H

#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

namespace scipp::core {

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

#endif // SCIPP_CORE_TRANSFORM_RESIZE_H
