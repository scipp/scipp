// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/util.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"

using namespace scipp;
using namespace scipp::dataset;

class BinnedVariableSizeOfTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 3};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 2}, std::pair{2, 4}});
  Variable buffer = makeVariable<double>(Dims{Dim::X}, Shape{4});
  Variable var = make_bins(indices, Dim::X, buffer);
};

class BinnedDataArraySizeOfTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data = makeVariable<double>(Dims{Dim::X}, Shape{4});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable var = make_bins(indices, Dim::X, buffer);
};

class BinnedDatasetSizeOfTest : public ::testing::Test {
protected:
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

TEST_F(BinnedVariableSizeOfTest, size_in_memory_of_bucketed_variable) {
  const auto &[indices_, dim_, buffer_] = var.constituents<bucket<Variable>>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(var), size_of(buffer_) + size_of(indices_));
}

TEST_F(BinnedVariableSizeOfTest, size_in_memory_of_sliced_bucketed_variable) {
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] =
      slice.constituents<bucket<Variable>>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(slice), size_of(buffer_) * 0.5 + size_of(indices_));
}

TEST_F(BinnedVariableSizeOfTest, empty_buffer) {
  Variable empty(var.slice(Slice(Dim::Y, 1)));
  const auto &[indices_, dim_, buffer_] =
      empty.constituents<bucket<Variable>>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(empty), size_of(indices_));
}

TEST_F(BinnedDataArraySizeOfTest, size_in_memory_of_bucketed_variable) {
  const auto &[indices_, dim_, buffer_] = var.constituents<bucket<DataArray>>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(var), size_of(buffer_) + size_of(indices_));
}

TEST_F(BinnedDataArraySizeOfTest, size_in_memory_of_sliced_bucketed_variable) {
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] =
      slice.constituents<bucket<DataArray>>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(slice), size_of(buffer_) * 0.5 + size_of(indices_));
}

TEST_F(BinnedDatasetSizeOfTest, size_in_memory_of_bucketed_variable) {
  buffer.setCoord(Dim::X, column);
  Variable var = make_bins(indices, Dim::X, buffer);
  const auto &[indices_, dim_, buffer_] = var.constituents<bucket<Dataset>>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(var), size_of(buffer_) + size_of(indices_));
}

TEST_F(BinnedDatasetSizeOfTest, size_in_memory_of_sliced_bucketed_variable) {
  buffer.setCoord(Dim::X, column);
  Variable var = make_bins(indices, Dim::X, buffer);
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] = slice.constituents<bucket<Dataset>>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(slice), size_of(buffer_) * 0.5 + size_of(indices_));
}
