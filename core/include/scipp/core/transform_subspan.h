// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_TRANSFORM_SUBSPAN_H
#define SCIPP_CORE_TRANSFORM_SUBSPAN_H

#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

namespace scipp::core {

namespace transform_subspan_detail {
static constexpr auto erase = [](Dimensions dims, const Dim dim) {
  dims.erase(dim);
  return dims;
};
static constexpr auto need_subspan = [](const VariableConstProxy &var,
                                        const Dim dim) {
  return !var.dims().sparse() && var.dims().contains(dim);
};

static constexpr auto maybe_subspan = [](VariableConstProxy &var,
                                         const Dim dim) {
  auto ret = std::make_unique<Variable>();
  if (need_subspan(var, dim)) {
    *ret = subspan_view(var, dim);
    var = *ret;
  }
  return std::move(ret);
};
} // namespace transform_subspan_detail

template <class Types, class Op, class... Var>
[[nodiscard]] Variable transform_subspan_impl(const Dim dim,
                                              const scipp::index size, Op op,
                                              Var... var) {
  using namespace transform_subspan_detail;

  auto dims = merge(erase(var.dims(), dim)...);
  dims.addInner(dim, size);
  // This will cause failure below unless the output type (first type in inner
  // tuple) is the same in all inner tuples.
  using OutT =
      typename std::tuple_element_t<0,
                                    std::tuple_element_t<0, Types>>::value_type;
  Variable out =
      std::is_base_of_v<transform_flags::expect_variance_arg_t<0>, Op>
          ? makeVariableWithVariances<OutT>(dims)
          : createVariable<OutT>(Dimensions{dims});

  const auto keep_subspan_vars_alive = std::array{maybe_subspan(var, dim)...};

  out.setUnit(op(var.unit()...));
  in_place<false>::transform_data(Types{}, op, subspan_view(out, dim), var...);
  return out;
}

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
[[nodiscard]] Variable
    transform_subspan(const Dim dim, const scipp::index size,
                      const VariableConstProxy &var1,
                      const VariableConstProxy &var2, Op op) {
      return transform_subspan_impl<Types>(dim, size, op, var1, var2);
    }

template <class Types, class Op>
[[nodiscard]] Variable
    transform_subspan(const Dim dim, const scipp::index size,
                      const VariableConstProxy &var1,
                      const VariableConstProxy &var2,
                      const VariableConstProxy &var3, Op op) {
      return transform_subspan_impl<Types>(dim, size, op, var1, var2, var3);
    }

} // namespace scipp::core

#endif // SCIPP_CORE_TRANSFORM_SUBSPAN_H
