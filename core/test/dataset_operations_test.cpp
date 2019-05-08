// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "dataset.h"
#include "dimensions.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

class DataProxyBinaryOperationsTest : public Dataset3DTest {};

TEST_F(DataProxyBinaryOperationsTest, plus_equals) {
  const auto target = dataset["data_zyx"];
  for (const auto & [ name, item ] : dataset) {
    static_cast<void>(name);
    auto reference_values = target.values() + item.values();
    auto reference_variances = target.variances();
    if (item.hasVariances())
      reference_variances += item.variances();

    ASSERT_NO_THROW(target += item);

    EXPECT_EQ(target.values(), reference_values);
    EXPECT_EQ(target.variances(), reference_variances);
  }
}

TEST_F(DataProxyBinaryOperationsTest, plus_equals_no_lhs_variance) {
  const auto target = dataset["data_xyz"];
  for (const auto & [ name, item ] : dataset) {
    static_cast<void>(name);
    auto reference_values = target.values() + item.values();
    if (item.hasVariances()) {
      ASSERT_ANY_THROW(target += item);
    } else {
      ASSERT_NO_THROW(target += item);
      EXPECT_EQ(target.values(), reference_values);
      EXPECT_FALSE(target.hasVariances());
    }
  }
}

TEST_F(DataProxyBinaryOperationsTest, times_equals) {
  const auto target = dataset["data_zyx"];
  for (const auto & [ name, item ] : dataset) {
    static_cast<void>(name);
    auto reference_values = target.values() * item.values();
    auto reference_variances =
        target.variances() * (item.values() * item.values());
    if (item.hasVariances())
      reference_variances +=
          (target.values() * target.values()) * item.variances();

    ASSERT_NO_THROW(target *= item);

    EXPECT_EQ(target.values(), reference_values);
    EXPECT_EQ(target.variances(), reference_variances);
  }
}

TEST_F(DataProxyBinaryOperationsTest, times_equals_no_lhs_variance) {
  const auto target = dataset["data_xyz"];
  for (const auto & [ name, item ] : dataset) {
    static_cast<void>(name);
    auto reference_values = target.values() * item.values();
    if (item.hasVariances()) {
      ASSERT_ANY_THROW(target *= item);
    } else {
      ASSERT_NO_THROW(target *= item);
      EXPECT_EQ(target.values(), reference_values);
      EXPECT_FALSE(target.hasVariances());
    }
  }
}
