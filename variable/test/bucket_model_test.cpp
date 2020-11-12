// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/buckets.h"

using namespace scipp;
using namespace scipp::variable;

TEST(BucketTest, member_types) {
  static_assert(std::is_same_v<bucket<Variable>::element_type, VariableView>);
  static_assert(
      std::is_same_v<bucket<Variable>::const_element_type, VariableConstView>);
}

using Model = DataModel<bucket<Variable>>;

class BucketModelTest : public ::testing::Test {
protected:
  Variable indices = makeVariable<index_pair>(
      Dims{Dim::Y}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  auto make_indices(
      const std::vector<std::pair<scipp::index, scipp::index>> &is) const {
    return makeVariable<std::pair<scipp::index, scipp::index>>(
        Dims{Dim::Y}, Shape{is.size()}, Values(is));
  }
};

TEST_F(BucketModelTest, construct) {
  Model model(indices, Dim::X, buffer);
  EXPECT_THROW(Model(indices, Dim::Y, buffer), except::DimensionError);
}

TEST_F(BucketModelTest, construct_empty_range) {
  auto empty = make_indices({{0, 2}, {2, 2}});
  EXPECT_NO_THROW(Model(empty, Dim::X, buffer));
}

TEST_F(BucketModelTest, construct_negative_range_fail) {
  auto overlapping = make_indices({{0, 2}, {2, 1}});
  EXPECT_THROW(Model(overlapping, Dim::X, buffer), except::SliceError);
}

TEST_F(BucketModelTest, construct_overlapping_fail) {
  auto overlapping = make_indices({{0, 3}, {2, 4}});
  EXPECT_THROW(Model(overlapping, Dim::X, buffer), except::SliceError);
}

TEST_F(BucketModelTest, construct_before_begin_fail) {
  auto before_begin = make_indices({{-1, 2}, {2, 4}});
  EXPECT_THROW(Model(before_begin, Dim::X, buffer), except::SliceError);
}

TEST_F(BucketModelTest, construct_beyond_end_fail) {
  auto beyond_end = make_indices({{0, 2}, {2, 5}});
  EXPECT_THROW(Model(beyond_end, Dim::X, buffer), except::SliceError);
}

TEST_F(BucketModelTest, dtype) {
  Model model(indices, Dim::X, buffer);
  EXPECT_NE(model.dtype(), buffer.dtype());
  EXPECT_EQ(model.dtype(), dtype<bucket<Variable>>);
}

TEST_F(BucketModelTest, variances) {
  Model model(indices, Dim::X, buffer);
  EXPECT_FALSE(model.hasVariances());
  EXPECT_THROW(model.setVariances(Variable(buffer)), except::VariancesError);
  EXPECT_FALSE(model.hasVariances());
}

TEST_F(BucketModelTest, comparison) {
  EXPECT_EQ(Model(indices, Dim::X, buffer), Model(indices, Dim::X, buffer));
  EXPECT_NE(Model(Variable(indices.slice({Dim::Y, 0})), Dim::X, buffer),
            Model(Variable(indices.slice({Dim::Y, 0, 1})), Dim::X, buffer));
  auto indices2 = indices;
  indices2.values<std::pair<scipp::index, scipp::index>>()[0] = {0, 1};
  EXPECT_NE(Model(indices, Dim::X, buffer), Model(indices2, Dim::X, buffer));
  auto buffer2 = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                      Values{1, 2, 3, 4});
  auto indices3 = make_indices({{0, 1}, {1, 2}});
  EXPECT_NE(Model(indices3, Dim::X, buffer2), Model(indices3, Dim::Y, buffer2));
  EXPECT_NE(Model(indices3, Dim::X, buffer), Model(indices3, Dim::X, buffer2));
}

TEST_F(BucketModelTest, clone) {
  Model model(indices, Dim::X, buffer);
  const auto copy = model.clone();
  EXPECT_EQ(dynamic_cast<const Model &>(*copy), model);
}

TEST_F(BucketModelTest, values) {
  Model model(indices, Dim::X, buffer);
  core::ElementArrayViewParams params(0, indices.dims(), indices.dims(), {});
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 0, 2}));
  EXPECT_EQ(*(model.values(params).begin() + 1), buffer.slice({Dim::X, 2, 4}));
  (*model.values(params).begin()) += 2.0 * units::one;
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 2, 4}));
}

TEST_F(BucketModelTest, values_const) {
  const Model model(indices, Dim::X, buffer);
  core::ElementArrayViewParams params(0, indices.dims(), indices.dims(), {});
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 0, 2}));
  EXPECT_EQ(*(model.values(params).begin() + 1), buffer.slice({Dim::X, 2, 4}));
}

TEST_F(BucketModelTest, values_non_range) {
  auto i = make_indices({{2, 4}, {0, -1}});
  Model model(i, Dim::X, buffer);
  core::ElementArrayViewParams params(0, i.dims(), i.dims(), {});
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 2, 4}));
  EXPECT_EQ(*(model.values(params).begin() + 1), buffer.slice({Dim::X, 0}));
}

TEST_F(BucketModelTest, out_of_order_indices) {
  auto reverse = make_indices({{2, 4}, {0, 2}});
  Model model(reverse, Dim::X, buffer);
  core::ElementArrayViewParams params(0, reverse.dims(), reverse.dims(), {});
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 2, 4}));
  EXPECT_EQ(*(model.values(params).begin() + 1), buffer.slice({Dim::X, 0, 2}));
}

class NonOwningBucketModelTest : public BucketModelTest {};

TEST_F(NonOwningBucketModelTest, buffer_is_view) {
  DataModel<bucket<VariableView>> model(indices, Dim::X, buffer);
  core::ElementArrayViewParams params(0, indices.dims(), indices.dims(), {});
  (*model.values(params).begin()) += 2.0 * units::one;
  EXPECT_EQ(buffer, makeVariable<double>(buffer.dims(), Values{3, 4, 3, 4}));
}

TEST_F(NonOwningBucketModelTest, indices_is_view) {
  DataModel<bucket<VariableView>> model(indices, Dim::X, buffer);
  EXPECT_EQ(model.indices(), indices);
  indices.values<index_pair>()[0] = std::pair{1, 2};
  EXPECT_EQ(model.indices(), indices);
}
