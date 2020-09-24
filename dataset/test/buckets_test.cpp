// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bucket.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::dataset;

using Model = variable::DataModel<bucket<DataArray>>;

class DataArrayBucketTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable column =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  DataArray buffer = DataArray(column, {{Dim::X, column + column}});
  Variable var{std::make_unique<Model>(indices, Dim::X, buffer)};
};

TEST_F(DataArrayBucketTest, concatenate) {
  const auto result = buckets::concatenate(var, var);
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 4}, std::pair{4, 8}});
  Variable column = makeVariable<double>(Dims{Dim::X}, Shape{8},
                                         Values{1, 2, 1, 2, 3, 4, 3, 4});
  DataArray buffer = DataArray(column, {{Dim::X, column + column}});
  EXPECT_EQ(result, Variable(std::make_unique<Model>(indices, Dim::X, buffer)));
}
