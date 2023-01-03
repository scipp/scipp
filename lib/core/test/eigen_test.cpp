// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/core/has_eval.h"

using namespace scipp::core;

TEST(EigenTest, has_eval) {
  // has_eval is used in variable/transform.h to detect Eigen types, ensure it
  // works for all types we use. Currently only Vector3d and Matrix3d.
  Eigen::Vector3d vec;
  ASSERT_FALSE(has_eval_v<double>);
  ASSERT_TRUE(has_eval_v<Eigen::Vector3d>);
  ASSERT_TRUE(has_eval_v<Eigen::Matrix3d>);
  ASSERT_TRUE(has_eval_v<std::decay_t<decltype(vec + vec)>>);
}

TEST(EigenTest, is_structured) {
  EXPECT_FALSE(is_structured(dtype<double>));
  EXPECT_TRUE(is_structured(dtype<Eigen::Vector3d>));
  EXPECT_TRUE(is_structured(dtype<Eigen::Matrix3d>));
}
