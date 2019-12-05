// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Igor Gudich
#include <cmath>

#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/tag_util.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

#include "operators.h"

namespace scipp::core {

struct MakeVariableWithType {
  template <class T> struct Maker {
    static Variable apply(const VariableConstProxy &parent) {
      return transform<double, float, int64_t, int32_t, bool>(
          parent,
          overloaded{
              [](const units::Unit &x) { return x; },
              [](const auto &x) {
                if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
                  return ValueAndVariance<T>{static_cast<T>(x.value),
                                             static_cast<T>(x.variance)};
                else
                  return static_cast<T>(x);
              }});
    }
  };
  static Variable make(const VariableConstProxy &var, DType type) {
    return CallDType<double, float, int64_t, int32_t, bool>::apply<Maker>(type,
                                                                          var);
  }
};

Variable astype(const VariableConstProxy &var, DType type) {
  return type == var.dtype() ? Variable(var)
                             : MakeVariableWithType::make(var, type);
}
} // namespace scipp::core
