// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

#include "dataset_test_common.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

TEST(DataArrayTest, construct) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();

  DataArray array(dataset["data_xyz"]);
  EXPECT_EQ(array, dataset["data_xyz"]);
  // Comparison ignores the name, so this is tested separately.
  EXPECT_EQ(array.name(), "data_xyz");
}

TEST(DataArrayTest, sum_dataset_columns_via_DataArray) {
  DatasetFactory3D factory;
  auto dataset = factory.make();

  DataArray array(dataset["data_zyx"]);
  auto sum = array + dataset["data_xyz"];

  dataset["data_zyx"] += dataset["data_xyz"];

  // Direction compare fails since binary operation do not propagate attributes.
  EXPECT_NE(sum, dataset["data_zyx"]);

  dataset.setData("sum", sum);
  EXPECT_EQ(dataset["sum"], dataset["data_zyx"]);
}

TEST(DataArrayTest, rebin) {
  DataArray a(makeVariable<double>({{Dim::Y, 2}, {Dim::X, 4}}, units::counts,
                                   {1, 2, 3, 4, 5, 6, 7, 8}),
              {{Dim::X, makeVariable<double>({Dim::X, 5}, {1, 2, 3, 4, 5})}},
              {});
  auto edges = makeVariable<double>({Dim::X, 3}, {1, 3, 5});
  DataArray expected(makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                          units::counts, {3, 7, 11, 15}),
                     {{Dim::X, edges}}, {});

  ASSERT_EQ(rebin(a, Dim::X, edges), expected);
}

TEST(DataArrayTest, rebin_with_variances) {
  DataArray a(makeVariable<double>({{Dim::Y, 2}, {Dim::X, 4}}, units::counts,
                                   {1, 2, 3, 4, 5, 6, 7, 8},
                                   {9, 10, 11, 12, 13, 14, 15, 16}),
              {{Dim::X, makeVariable<double>({Dim::X, 5}, {1, 2, 3, 4, 5})}},
              {});
  auto edges = makeVariable<double>({Dim::X, 3}, {1, 3, 5});
  DataArray expected(makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                          units::counts, {3, 7, 11, 15},
                                          {19, 23, 27, 31}),
                     {{Dim::X, edges}}, {});

  ASSERT_EQ(rebin(a, Dim::X, edges), expected);
}
