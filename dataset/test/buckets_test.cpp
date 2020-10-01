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
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable var{std::make_unique<Model>(indices, Dim::X, buffer)};
};

TEST_F(DataArrayBucketTest, concatenate) {
  const auto result = buckets::concatenate(var, var * (3.0 * units::one));
  Variable out_indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 4}, std::pair{4, 8}});
  Variable out_data = makeVariable<double>(Dims{Dim::X}, Shape{8},
                                           Values{1, 2, 3, 6, 3, 4, 9, 12});
  Variable out_x = makeVariable<double>(Dims{Dim::X}, Shape{8},
                                        Values{2, 4, 2, 4, 6, 8, 6, 8});
  DataArray out_buffer = DataArray(out_data, {{Dim::X, out_x}});
  EXPECT_EQ(result,
            Variable(std::make_unique<Model>(out_indices, Dim::X, out_buffer)));

  // "in-place" append gives same as concatenate
  buckets::append(var, var * (3.0 * units::one));
  EXPECT_EQ(result, var);
  buckets::append(var, -var);
}

TEST_F(DataArrayBucketTest, concatenate_with_broadcast) {
  auto var2 = var;
  var2.rename(Dim::Y, Dim::Z);
  var2 *= 3.0 * units::one;
  const auto result = buckets::concatenate(var, var2);
  Variable out_indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::Y, Dim::Z}, Shape{2, 2},
      Values{std::pair{0, 4}, std::pair{4, 8}, std::pair{8, 12},
             std::pair{12, 16}});
  Variable out_data = makeVariable<double>(
      Dims{Dim::X}, Shape{16},
      Values{1, 2, 3, 6, 1, 2, 9, 12, 3, 4, 3, 6, 3, 4, 9, 12});
  Variable out_x = makeVariable<double>(
      Dims{Dim::X}, Shape{16},
      Values{2, 4, 2, 4, 2, 4, 6, 8, 6, 8, 2, 4, 6, 8, 6, 8});
  DataArray out_buffer = DataArray(out_data, {{Dim::X, out_x}});
  EXPECT_EQ(result,
            Variable(std::make_unique<Model>(out_indices, Dim::X, out_buffer)));

  // Broadcast not possible for in-place append
  EXPECT_THROW(buckets::append(var, var2), except::DimensionMismatchError);
}
