// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

#include "test_random.h"

namespace scipp::testing {
[[nodiscard]] scipp::variable::Variable Random::make_variable(
    const scipp::Dimensions &dims, const scipp::core::DType dtype,
    const scipp::units::Unit unit, const bool with_variances) {
  using namespace scipp;
  if (dtype == core::dtype<double>) {
    if (with_variances) {
      return makeVariable<double>(
          dims, Values(generate<double>(dims.volume())),
          Variances(generate<double>(dims.volume(),
                                     [](double x) { return std::abs(x); })),
          unit);
    }
    return makeVariable<double>(dims, Values(generate<double>(dims.volume())),
                                unit);
  }
  assert(!with_variances); // the other dtypes do not support variances
  if (dtype == core::dtype<int64_t>) {
    return makeVariable<int64_t>(dims, Values(generate<int64_t>(dims.volume())),
                                 unit);
  }
  if (dtype == core::dtype<int32_t>) {
    return makeVariable<int32_t>(dims, Values(generate<int32_t>(dims.volume())),
                                 unit);
  }
  if (dtype == core::dtype<core::time_point>) {
    return makeVariable<core::time_point>(
        dims, Values(generate<core::time_point>(dims.volume())), unit);
  }
  throw std::invalid_argument(
      "Random variable generation is not implemented for dtype " +
      to_string(dtype));
}

scipp::Variable makeRandom(const scipp::Dimensions &dims, const double min,
                           const double max) {
  using namespace scipp;
  Random rand(min, max);
  return makeVariable<double>(Dimensions{dims}, Values(rand(dims.volume())));
}
} // namespace scipp::testing