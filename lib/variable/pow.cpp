// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "scipp/variable/pow.h"

#include "scipp/core/element/math.h"
#include "scipp/core/except.h"
#include "scipp/core/tag_util.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

namespace {
template <class V>
Variable pow_do_transform(V &&base, const Variable &exponent,
                          const bool in_place) {
  if (!in_place) {
    return variable::transform(base, exponent, element::pow, "pow");
  } else {
    if constexpr (std::is_const_v<std::remove_reference_t<V>>) {
      return variable::transform(base, exponent, element::pow, "pow");
    } else {
      variable::transform_in_place(base, base, exponent, element::pow_in_place,
                                   "pow");
      return std::forward<V>(base);
    }
  }
}

template <class T> struct PowUnit {
  static sc_units::Unit apply(const sc_units::Unit base_unit,
                              const Variable &exponent) {
    const auto exp_val = exponent.value<T>();
    if constexpr (std::is_floating_point_v<T>) {
      if (static_cast<T>(static_cast<int64_t>(exp_val)) != exp_val) {
        throw except::UnitError("Powers of dimension-full variables must be "
                                "integers or integer valued floats. Got " +
                                std::to_string(exp_val) + ".");
      }
    }
    return pow(base_unit, exp_val);
  }
};

template <class V>
Variable pow_handle_unit(V &&base, const Variable &exponent,
                         const bool in_place) {
  if (const auto exp_unit = variableFactory().elem_unit(exponent);
      exp_unit != sc_units::one) {
    throw except::UnitError("Powers must be dimensionless, got exponent.unit=" +
                            to_string(exp_unit) + ".");
  }

  const auto base_unit = variableFactory().elem_unit(base);
  if (base_unit == sc_units::one) {
    return pow_do_transform(std::forward<V>(base), exponent, in_place);
  }
  if (exponent.dims().ndim() != 0) {
    throw except::DimensionError("Exponents must be scalar if the base is not "
                                 "dimensionless. Got base.unit=" +
                                 to_string(base_unit) + " and exponent.dims=" +
                                 to_string(exponent.dims()) + ".");
  }

  Variable res = in_place ? std::forward<V>(base) : copy(std::forward<V>(base));
  variableFactory().set_elem_unit(res, sc_units::one);
  pow_do_transform(res, exponent, true);
  variableFactory().set_elem_unit(
      res, core::CallDType<double, float, int64_t, int32_t>::apply<PowUnit>(
               exponent.dtype(), base_unit, exponent));
  return res;
}

bool has_negative_value(const Variable &var) {
  return astype(min(var), dtype<int64_t>, CopyPolicy::TryAvoid)
             .value<int64_t>() < 0l;
}

template <class V>
Variable pow_handle_dtype(V &&base, const Variable &exponent,
                          const bool in_place) {
  if (is_bins(exponent)) {
    throw std::invalid_argument("Binned exponents are not supported by pow.");
  }
  if (!is_int(base.dtype())) {
    return pow_handle_unit(std::forward<V>(base), exponent, in_place);
  }
  if (is_int(exponent.dtype())) {
    if (has_negative_value(exponent)) {
      throw std::invalid_argument(
          "Integers to negative powers are not allowed.");
    }
    return pow_handle_unit(std::forward<V>(base), exponent, in_place);
  }
  // Base has integer dtype but exponent does not.
  return pow_handle_unit(astype(base, exponent.dtype()), exponent, true);
}
} // namespace

Variable pow(const Variable &base, const Variable &exponent) {
  return pow_handle_dtype(base.broadcast(merge(base.dims(), exponent.dims())),
                          exponent, false);
}

Variable &pow(const Variable &base, const Variable &exponent, Variable &out) {
  const auto target_dims = merge(base.dims(), exponent.dims());
  core::expect::equals(target_dims, out.dims());
  copy(astype(base, out.dtype(), CopyPolicy::TryAvoid), out);
  pow_handle_dtype(out, exponent, true);
  return out;
}
} // namespace scipp::variable
