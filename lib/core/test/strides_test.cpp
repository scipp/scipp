// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/strides.h"

using namespace scipp;

TEST(StridesTest, resize_keeps_old_elements) {
  Strides strides{2, 3};
  strides.resize(3);
  ASSERT_EQ(strides, Strides({2, 3, 0}));
}

TEST(StridesTest, clear_results_in_empty_strides) {
  Strides strides{2, 3};
  strides.clear();
  ASSERT_EQ(strides, Strides());
}
