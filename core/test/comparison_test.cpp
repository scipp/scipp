// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/comparison.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(Comparison, is_approx) {
  const double delta = 0.1;
  const auto a =
      makeVariable<double>({Dim::X, 2}, units::m, {1.0 + delta, 2.0});
  const auto b = makeVariable<double>({Dim::X, 2}, units::m, {1.0, 2.0});
  ASSERT_TRUE(approx_same(a, b, delta + 0.001));
  ASSERT_FALSE(approx_same(a, b, delta - 0.001));
}
