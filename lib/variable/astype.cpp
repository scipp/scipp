// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include <array>
#include <cmath>
#include <type_traits>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/tag_util.h"
#include "scipp/core/transform_common.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

struct MakeVariableWithType {
  using AllSourceTypes = std::tuple<double, float, int64_t, int32_t, bool>;

  template <class T> struct Maker {
    template <size_t I, class... Types> constexpr static auto source_types() {
      if constexpr (I == std::tuple_size_v<AllSourceTypes>) {
        return std::tuple<Types...>{};
      } else {
        using Next = typename std::tuple_element<I, AllSourceTypes>::type;
        if constexpr (std::is_same_v<Next, T>) {
          return source_types<I + 1, Types...>();
        } else {
          return source_types<I + 1, Types..., Next>();
        }
      }
    }

    template <class... SourceTypes>
    static Variable apply_impl(const Variable &parent,
                               std::tuple<SourceTypes...>) {
      using namespace core::transform_flags;
      constexpr auto expect_input_variances =
          conditional_flag<!core::canHaveVariances<T>()>(
              expect_no_variance_arg<0>);
      return transform<SourceTypes...>(
          parent,
          overloaded{
              expect_input_variances, [](const sc_units::Unit &x) { return x; },
              [](const auto &x) {
                if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                  return ValueAndVariance<T>{static_cast<T>(x.value),
                                             static_cast<T>(x.variance)};
                else
                  return static_cast<T>(x);
              }},
          "astype");
    }

    static Variable apply(const Variable &parent) {
      return apply_impl(parent, source_types<0>());
    }
  };

  static Variable make(const Variable &var, DType type) {
    return core::CallDType<double, float, int64_t, int32_t, bool>::apply<Maker>(
        type, var);
  }
};

Variable astype(const Variable &var, DType type, const CopyPolicy copy) {
  return type == variableFactory().elem_dtype(var)
             ? (copy == CopyPolicy::TryAvoid ? var : variable::copy(var))
             : MakeVariableWithType::make(var, type);
}

DType common_type(const Variable &a, const Variable &b) {
  return common_type(variableFactory().elem_dtype(a),
                     variableFactory().elem_dtype(b));
}

} // namespace scipp::variable
