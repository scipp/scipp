// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#include "scipp/core/element/comparison.h"
#include "scipp/variable/accumulate.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/math.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

using namespace scipp::core;

namespace scipp::variable {

namespace {
Variable _values(Variable &&in) { return in.hasVariances() ? values(in) : in; }
} // namespace

Variable allclose(const Variable &a, const Variable &b, const Variable &rtol,
                  const Variable &atol, const NanComparisons equal_nans) {
  auto tol = atol + rtol * abs(b);
  if (a.hasVariances() && b.hasVariances()) {
    return allclose(values(a), values(b), rtol, atol, equal_nans) &
           allclose(stddevs(a), stddevs(b), rtol, atol, equal_nans);
  } else {
    constexpr auto isclose_equal_nan_out = overloaded{
        [](auto &&out, const auto &a, const auto &b, const auto tol) {
          out &= element::isclose_equal_nan(a, b, tol);
        },
        element::arg_list<std::tuple<bool, double, double, double>>};

    Variable result = makeVariable<bool>(Values{true});
    if (equal_nans == NanComparisons::Equal)
      accumulate_in_place(result, const_cast<Variable &>(a), b,
                          _values(std::move(tol)), isclose_equal_nan_out,
                          "allclose");
    else
      accumulate_in_place(result, const_cast<Variable &>(a), b,
                          _values(std::move(tol)), isclose_equal_nan_out,
                          "allclose");
    return result;
  }
}

Variable isclose(const Variable &a, const Variable &b, const Variable &rtol,
                 const Variable &atol, const NanComparisons equal_nans) {
  auto tol = atol + rtol * abs(b);
  if (a.hasVariances() && b.hasVariances()) {
    return isclose(values(a), values(b), rtol, atol, equal_nans) &
           isclose(stddevs(a), stddevs(b), rtol, atol, equal_nans);
  } else {
    if (equal_nans == NanComparisons::Equal)
      return variable::transform(a, b, _values(std::move(tol)),
                                 element::isclose_equal_nan, "isclose");
    else
      return variable::transform(a, b, _values(std::move(tol)),
                                 element::isclose, "isclose");
  }
}

} // namespace scipp::variable
