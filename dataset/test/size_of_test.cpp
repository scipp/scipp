// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bucket.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/util.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/buckets.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"

using namespace scipp;
using namespace scipp::dataset;

using Model = variable::DataModel<bucket<Variable>>;

class BucketVariableSizeOfTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 3};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 3}, std::pair{3, 4}});
  Variable buffer = makeVariable<double>(Dims{Dim::X}, Shape{4});
  Variable var{std::make_unique<Model>(indices, Dim::X, buffer)};
};

class BucketDataArraySizeOfTest : public ::testing::Test {
protected:
  using Model = variable::DataModel<bucket<DataArray>>;
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data = makeVariable<double>(Dims{Dim::X}, Shape{4});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable var{std::make_unique<Model>(indices, Dim::X, buffer)};
};

class BucketDatasetSizeOfTest : public ::testing::Test {
protected:
  using Model = variable::DataModel<bucket<Dataset>>;
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable column = makeVariable<double>(Dims{Dim::X}, Shape{4});
  Dataset buffer;
};

TEST(SizeOf, variable) {
  auto var = makeVariable<double>(Shape{4}, Dims{Dim::X});
  EXPECT_EQ(size_of(var), sizeof(double) * 4);

  auto var_with_variance = makeVariable<double>(
      Shape{1, 2}, Dims{Dim::X, Dim::Y}, Values{3, 4}, Variances{1, 2});

  EXPECT_EQ(size_of(var_with_variance), sizeof(double) * 4);

  auto sliced_view = var.slice(Slice(Dim::X, 0, 2));
  EXPECT_EQ(size_of(sliced_view), 2 * sizeof(double));
}

TEST(SizeOf, size_in_memory_for_non_trivial_dtype) {
  auto var = makeVariable<Eigen::Vector3d>(Shape{1, 1}, Dims{Dim::X, Dim::Y});
  EXPECT_EQ(size_of(var), sizeof(Eigen::Vector3d));
}

TEST(SizeOf, size_in_memory_sliced_variables) {
  auto var = makeVariable<double>(Shape{4}, Dims{Dim::X});
  auto sliced_view = var.slice(Slice(Dim::X, 0, 2));
  EXPECT_EQ(size_of(sliced_view), 2 * sizeof(double));
}

TEST_F(BucketVariableSizeOfTest, size_in_memory_of_bucketed_variable) {
  const auto &[indices_, dim_, buffer_] =
      VariableConstView(var).constituents<bucket<Variable>>();
  EXPECT_EQ(size_of(var), size_of(buffer_) + size_of(indices_));
}

TEST_F(BucketVariableSizeOfTest, size_in_memory_of_sliced_bucketed_variable) {
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] =
      slice.constituents<bucket<Variable>>();
  EXPECT_EQ(size_of(slice), size_of(buffer_) * 0.5 + size_of(indices_));
}

TEST_F(BucketDataArraySizeOfTest, size_in_memory_of_bucketed_variable) {
  const auto &[indices_, dim_, buffer_] =
      VariableConstView(var).constituents<bucket<DataArray>>();
  EXPECT_EQ(size_of(var), size_of(buffer_) + size_of(indices_));
}

TEST_F(BucketDataArraySizeOfTest, size_in_memory_of_sliced_bucketed_variable) {
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] =
      slice.constituents<bucket<DataArray>>();
  EXPECT_EQ(size_of(slice), size_of(buffer_) * 0.5 + size_of(indices_));
}

TEST_F(BucketDatasetSizeOfTest, size_in_memory_of_bucketed_variable) {
  buffer.coords().set(Dim::X, column);
  Variable var{std::make_unique<Model>(indices, Dim::X, buffer)};
  const auto &[indices_, dim_, buffer_] =
      VariableConstView(var).constituents<bucket<Dataset>>();
  EXPECT_EQ(size_of(var), size_of(buffer_) + size_of(indices_));
}

TEST_F(BucketDatasetSizeOfTest, size_in_memory_of_sliced_bucketed_variable) {
  buffer.coords().set(Dim::X, column);
  Variable var{std::make_unique<Model>(indices, Dim::X, buffer)};
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] = slice.constituents<bucket<Dataset>>();
  EXPECT_EQ(size_of(slice), size_of(buffer_) * 0.5 + size_of(indices_));
}
