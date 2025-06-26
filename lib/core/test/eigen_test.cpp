// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"

using namespace scipp::core;

TEST(EigenTest, is_structured) {
  EXPECT_FALSE(is_structured(dtype<double>));
  EXPECT_TRUE(is_structured(dtype<Eigen::Vector3d>));
  EXPECT_TRUE(is_structured(dtype<Eigen::Matrix3d>));
}
