// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "scipp/variable/pow.h"

#include "scipp/core/element/math.h"
#include "scipp/core/tag_util.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

namespace {
template <class V>
Variable pow_do_transform(V &&base, const Variable &exponent) {
  if constexpr (std::is_const_v<std::remove_reference_t<V>>) {
    return variable::transform(base, exponent, element::pow, "pow");
  } else {
    variable::transform_in_place(base, base, exponent, element::pow_in_place,
                                 "pow");
    return std::forward<V>(base);
  }
}

template <class T> struct PowUnit {
  static units::Unit apply(const units::Unit base_unit,
                           const Variable &exponent) {
    const auto val = exponent.value<T>();
    if constexpr (std::is_floating_point_v<T>) {
      if (static_cast<T>(static_cast<int64_t>(val)) != val) {
        throw except::UnitError("Powers of dimension-full variables must be "
                                "integers or integer values floats. Got " +
                                std::to_string(val) + ".");
      }
    }
    return pow(base_unit, exponent.value<T>());
  }
};

template <class V>
Variable pow_handle_unit(V &&base, const Variable &exponent,
                         const bool safe_for_in_place) {
  if (exponent.unit() != units::one) {
    throw except::UnitError("Powers must be dimensionless, got exponent.unit=" +
                            to_string(exponent.unit()) + ".");
  }

  const auto base_unit = base.unit();
  if (base_unit == units::one) {
    return pow_do_transform(std::forward<V>(base), exponent);
  }
  if (exponent.dims().ndim() != 0) {
    throw except::DimensionError("Exponents must be scalar if the base is not "
                                 "dimensionless. Got base.unit=" +
                                 to_string(base_unit) + " and exponent.dims=" +
                                 to_string(exponent.dims()) + ".");
  }

  Variable res =
      safe_for_in_place ? std::forward<V>(base) : copy(std::forward<V>(base));
  res.setUnit(units::one);
  pow_do_transform(res, exponent);
  res.setUnit(core::CallDType<double, int64_t>::apply<PowUnit>(
      exponent.dtype(), base_unit, exponent));
  return res;
}

Variable pow_handle_dtype(const Variable &base, const Variable &exponent) {
  if (!is_int(base.dtype())) {
    return pow_handle_unit(base, exponent, false);
  }
  if (is_int(exponent.dtype())) {
    if (astype(min(exponent), dtype<int64_t>, CopyPolicy::TryAvoid)
            .value<int64_t>() < 0l) {
      throw std::invalid_argument(
          "Integers to negative powers are not allowed.");
    }
    return pow_handle_unit(base, exponent, false);
  }
  // Base has integer dtype but exponent does not.
  return pow_handle_unit(astype(base, exponent.dtype()), exponent, true);
}
} // namespace

Variable pow(const Variable &base, const Variable &exponent) {
  return pow_handle_dtype(base.broadcast(merge(base.dims(), exponent.dims())),
                          exponent);
}
} // namespace scipp::variable
