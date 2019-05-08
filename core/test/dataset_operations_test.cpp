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
  auto reference_values =
      dataset["data_x"].values() + dataset["data_x"].values();
  auto reference_variances =
      dataset["data_x"].variances() + dataset["data_x"].variances();

  ASSERT_NO_THROW(dataset["data_x"] += dataset["data_x"]);

  EXPECT_EQ(dataset["data_x"].values(), reference_values);
  EXPECT_EQ(dataset["data_x"].variances(), reference_variances);
}

TEST_F(DataProxyBinaryOperationsTest, times_equals) {
  const auto target = dataset["data_zyx"];
  for (const auto & [ name, item ] : dataset) {
    static_cast<void>(name);
    if (!item.hasVariances())
      continue;
    auto reference_values = target.values() * item.values();
    auto reference_variances =
        target.variances() * (item.values() * item.values()) +
        (target.values() * target.values()) * item.variances();

    ASSERT_NO_THROW(target *= item);

    EXPECT_EQ(target.values(), reference_values);
    EXPECT_EQ(target.variances(), reference_variances);
  }
}
