// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bucket_model.tcc"

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
  Dimensions dims{Dim::X, 4};
  element_array<std::pair<scipp::index, scipp::index>> buckets{{0, 2}, {2, 4}};
  Variable buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
};

TEST_F(BucketModelTest, construct) {
  Model model(dims, buckets, Dim::X, buffer);
  EXPECT_THROW(Model(dims, buckets, Dim::Y, buffer), except::DimensionError);
}

TEST_F(BucketModelTest, dtype) {
  Model model(dims, buckets, Dim::X, buffer);
  EXPECT_NE(model.dtype(), buffer.dtype());
  EXPECT_EQ(model.dtype(), dtype<bucket<Variable>>);
}

TEST_F(BucketModelTest, variances) {
  Model model(dims, buckets, Dim::X, buffer);
  EXPECT_FALSE(model.hasVariances());
  EXPECT_THROW(model.setVariances(Variable(buffer)), except::VariancesError);
  EXPECT_FALSE(model.hasVariances());
}

TEST_F(BucketModelTest, comparison){
  EXPECT_EQ(Model(dims, buckets, Dim::X, buffer),
            Model(dims, buckets, Dim::X, buffer));
  EXPECT_NE(Model(dims, buckets, Dim::X, buffer),
            Model({{Dim::X, Dim::Y}, {2, 2}}, buckets, Dim::X, buffer));
  auto buckets2 = buckets;
  buckets2.data()[0] = {0, 1};
  EXPECT_NE(Model(dims, buckets, Dim::X, buffer),
            Model(dims, buckets2, Dim::X, buffer));
  auto buffer2 = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                      Values{1, 2, 3, 4});
  EXPECT_NE(Model(dims, buckets, Dim::X, buffer2),
            Model(dims, buckets, Dim::Y, buffer2));
  EXPECT_NE(Model(dims, buckets, Dim::X, buffer),
            Model(dims, buckets, Dim::X, buffer2));
}

TEST_F(BucketModelTest, clone) {
  Model model(dims, buckets, Dim::X, buffer);
  const auto copy = model.clone();
  EXPECT_EQ(dynamic_cast<const Model &>(*copy), model);
}

TEST_F(BucketModelTest, values) {
  Model model(dims, buckets, Dim::X, buffer);
  EXPECT_EQ(*(model.values().begin() + 0), buffer.slice({Dim::X, 0, 2}));
  EXPECT_EQ(*(model.values().begin() + 1), buffer.slice({Dim::X, 2, 4}));
  (*model.values().begin()) += 2.0 * units::one;
  EXPECT_EQ(*(model.values().begin() + 0), buffer.slice({Dim::X, 2, 4}));
}

TEST_F(BucketModelTest, values_const) {
  const Model model(dims, buckets, Dim::X, buffer);
  EXPECT_EQ(*(model.values().begin() + 0), buffer.slice({Dim::X, 0, 2}));
  EXPECT_EQ(*(model.values().begin() + 1), buffer.slice({Dim::X, 2, 4}));
}
