// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/arithmetic.h"

#include "dataset_test_common.h"
#include "test_macros.h"

using namespace scipp;

TEST(DataArrayArithmeticTest, fail_op_non_matching_coords) {
  auto coord_1 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  auto coord_2 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  DataArray da_1(data, {{Dim::X, coord_1}, {Dim::Y, data}});
  DataArray da_2(data, {{Dim::X, coord_2}, {Dim::Y, data}});
  EXPECT_THROW_DISCARD(da_1 + da_2, except::CoordMismatchError);
  EXPECT_THROW_DISCARD(da_1 - da_2, except::CoordMismatchError);
}

TEST(DataArrayArithmeticTest, sum_dataset_columns_via_DataArray) {
  DatasetFactory3D factory;
  auto dataset = factory.make();

  DataArray array(dataset["data_zyx"]);
  auto sum = array + dataset["data_xyz"];

  dataset["data_zyx"] += dataset["data_xyz"];

  // This would fail if the data items had attributes, since += preserves them
  // but + does not.
  EXPECT_EQ(sum, dataset["data_zyx"]);
}
