// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bin_array_model.h"
#include "scipp/variable/bins.h"

#include "test_macros.h"

using namespace scipp;
using namespace scipp::variable;

TEST(BucketTest, member_types) {
  static_assert(std::is_same_v<bucket<Variable>::element_type, Variable>);
  static_assert(std::is_same_v<bucket<Variable>::const_element_type, Variable>);
}

using Model = BinArrayModel<Variable>;

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
  // No validation
  EXPECT_NO_THROW(Model(indices.data_handle(), Dim::Y, buffer));
  // make_bins validates
  EXPECT_THROW_DISCARD(make_bins(indices, Dim::Y, buffer),
                       except::DimensionError);
}

TEST_F(BucketModelTest, construct_empty_range) {
  auto empty = make_indices({{0, 2}, {2, 2}}).data_handle();
  EXPECT_NO_THROW(Model(empty, Dim::X, buffer));
}

TEST_F(BucketModelTest, construct_negative_range_fail) {
  auto overlapping = make_indices({{0, 2}, {2, 1}});
  EXPECT_THROW_DISCARD(make_bins(overlapping, Dim::X, buffer),
                       except::SliceError);
}

TEST_F(BucketModelTest, construct_overlapping_fail) {
  auto overlapping = make_indices({{0, 3}, {2, 4}});
  EXPECT_THROW_DISCARD(make_bins(overlapping, Dim::X, buffer),
                       except::SliceError);
}

TEST_F(BucketModelTest, construct_before_begin_fail) {
  auto before_begin = make_indices({{-1, 2}, {2, 4}});
  EXPECT_THROW_DISCARD(make_bins(before_begin, Dim::X, buffer),
                       except::SliceError);
}

TEST_F(BucketModelTest, construct_beyond_end_fail) {
  auto beyond_end = make_indices({{0, 2}, {2, 5}});
  EXPECT_THROW_DISCARD(make_bins(beyond_end, Dim::X, buffer),
                       except::SliceError);
}

TEST_F(BucketModelTest, dtype) {
  Model model(indices.data_handle(), Dim::X, buffer);
  EXPECT_NE(model.dtype(), buffer.dtype());
  EXPECT_EQ(model.dtype(), dtype<bucket<Variable>>);
}

TEST_F(BucketModelTest, variances) {
  Model model(indices.data_handle(), Dim::X, buffer);
  EXPECT_FALSE(model.has_variances());
  EXPECT_THROW(model.setVariances(Variable(buffer)), except::VariancesError);
  EXPECT_FALSE(model.has_variances());
}

TEST_F(BucketModelTest, comparison) {
  EXPECT_EQ(Model(indices.data_handle(), Dim::X, buffer),
            Model(indices.data_handle(), Dim::X, buffer));
  // The model has no concept of dims so these two cases cannot be distinguished
  EXPECT_EQ(
      Model(copy(indices.slice({Dim::Y, 0})).data_handle(), Dim::X, buffer),
      Model(copy(indices.slice({Dim::Y, 0, 1})).data_handle(), Dim::X, buffer));
  EXPECT_NE(
      Model(copy(indices.slice({Dim::Y, 1})).data_handle(), Dim::X, buffer),
      Model(copy(indices.slice({Dim::Y, 0, 1})).data_handle(), Dim::X, buffer));
  auto indices2 = copy(indices);
  indices2.values<std::pair<scipp::index, scipp::index>>()[0] = {0, 1};
  EXPECT_NE(Model(indices.data_handle(), Dim::X, buffer),
            Model(indices2.data_handle(), Dim::X, buffer));
  auto buffer2 = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                      Values{1, 2, 3, 4});
  auto indices3 = make_indices({{0, 1}, {1, 2}}).data_handle();
  EXPECT_NE(Model(indices3, Dim::X, buffer2), Model(indices3, Dim::Y, buffer2));
  EXPECT_NE(Model(indices3, Dim::X, buffer), Model(indices3, Dim::X, buffer2));
}

TEST_F(BucketModelTest, copy) {
  // This is used to implement clone(), which has to make a deep copy
  Model model(indices.data_handle(), Dim::X, buffer);
  const auto copied = copy(model);
  EXPECT_EQ(copied, model);
  EXPECT_NE(copied.indices(), model.indices()); // pointer comparison
  EXPECT_FALSE(copied.buffer().is_same(model.buffer()));
}

TEST_F(BucketModelTest, values) {
  Model model(indices.data_handle(), Dim::X, buffer);
  core::ElementArrayViewParams params(0, indices.dims(), Strides{1}, {});
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 0, 2}));
  EXPECT_EQ(*(model.values(params).begin() + 1), buffer.slice({Dim::X, 2, 4}));
  (*model.values(params).begin()) += 2.0 * sc_units::one;
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 2, 4}));
}

TEST_F(BucketModelTest, values_const) {
  const Model model(indices.data_handle(), Dim::X, buffer);
  core::ElementArrayViewParams params(0, indices.dims(), Strides{1}, {});
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 0, 2}));
  EXPECT_EQ(*(model.values(params).begin() + 1), buffer.slice({Dim::X, 2, 4}));
}

TEST_F(BucketModelTest, values_non_range) {
  auto i = make_indices({{2, 4}, {0, -1}});
  // The model would actually support this, but operations with such data do not
  // handle this case, so it is disabled.
  EXPECT_THROW_DISCARD(make_bins(i, Dim::X, buffer), except::SliceError);
}

TEST_F(BucketModelTest, out_of_order_indices) {
  auto reverse = make_indices({{2, 4}, {0, 2}}).data_handle();
  const Dimensions dims(Dim::Y, 2);
  Model model(reverse, Dim::X, copy(buffer));
  core::ElementArrayViewParams params(0, dims, Strides{1}, {});
  EXPECT_EQ(*(model.values(params).begin() + 0), buffer.slice({Dim::X, 2, 4}))
      << *(model.values(params).begin() + 0) << buffer.slice({Dim::X, 2, 4});
  EXPECT_EQ(*(model.values(params).begin() + 1), buffer.slice({Dim::X, 0, 2}));
}
