// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/rebin.h"
#include "scipp/dataset/reduction.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/arithmetic.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::dataset;

class AttributesTest : public ::testing::Test {
protected:
  const Variable scalar = makeVariable<double>(Values{1});
  const Variable varX =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 3});
  const Variable varZX = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                              Values{4, 5, 6, 7});
};

TEST_F(AttributesTest, dataset_item_attrs) {
  Dataset d;
  d.setData("a", varX);
  d["a"].coords().set(Dim("scalar"), scalar);
  d["a"].coords().set(Dim("x"), varX);
  d.coords().set(Dim("dataset_attr"), scalar);

  ASSERT_FALSE(d.coords().contains(Dim("scalar")));
  ASSERT_FALSE(d.coords().contains(Dim("x")));

  ASSERT_EQ(d["a"].unaligned_coords().size(), 2);
  ASSERT_TRUE(d["a"].unaligned_coords().contains(Dim("scalar")));
  ASSERT_TRUE(d["a"].unaligned_coords().contains(Dim("x")));
  ASSERT_TRUE(d["a"].aligned_coords().contains(Dim("dataset_attr")));
  ASSERT_FALSE(d["a"].unaligned_coords().contains(Dim("dataset_attr")));

  d["a"].coords().erase(Dim("scalar"));
  d["a"].coords().erase(Dim("x"));
  ASSERT_EQ(d["a"].unaligned_coords().size(), 0);
}

TEST_F(AttributesTest, slice_dataset_item_attrs) {
  Dataset d;
  d.setData("a", varZX);
  d["a"].coords().set(Dim("scalar"), scalar);
  d["a"].coords().set(Dim("x"), varX);

  // Same behavior as coord slicing:
  // - Lower-dimensional attrs are not hidden by slicing.
  // - Non-range slice hides attribute.
  // The alternative would be to handle attributes like data, but at least for
  // now coord-like handling appears to make more sense.
  ASSERT_TRUE(d["a"].slice({Dim::X, 0}).coords().contains(Dim("scalar")));
  ASSERT_FALSE(d["a"].slice({Dim::X, 0}).aligned_coords().contains(Dim("x")));
  ASSERT_TRUE(d["a"].slice({Dim::X, 0}).coords().contains(Dim("x")));
  ASSERT_TRUE(d["a"].slice({Dim::X, 0, 1}).coords().contains(Dim("scalar")));
  ASSERT_TRUE(d["a"].slice({Dim::X, 0, 1}).coords().contains(Dim("x")));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0}).coords().contains(Dim("scalar")));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0}).coords().contains(Dim("x")));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0, 1}).coords().contains(Dim("scalar")));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0, 1}).coords().contains(Dim("x")));
}

TEST_F(AttributesTest, binary_ops_matching_attrs_preserved) {
  Dataset d;
  d.setData("a", varX);
  d["a"].coords().set(Dim("a_attr"), scalar);

  for (const auto &result : {d + d, d - d, d * d, d / d}) {
    EXPECT_EQ(result["a"].coords(), d["a"].coords());
  }
}

TEST_F(AttributesTest, binary_ops_mismatching_attrs_dropped) {
  Dataset d1;
  d1.setData("a", varX);
  d1["a"].coords().set(Dim("a_attr"), scalar);
  Dataset d2;
  d2.setData("a", varX);
  d2["a"].coords().set(Dim("a_attr"), scalar + scalar); // mismatching content
  d2["a"].coords().set(Dim("a_attr2"), scalar);         // mismatching name

  for (const auto &result : {d1 + d2, d1 - d2, d1 * d2, d1 / d2}) {
    EXPECT_TRUE(result["a"].coords().empty());
  }
}

TEST_F(AttributesTest, binary_ops_in_place) {
  Dataset d1;
  d1.setData("a", varX);
  d1["a"].coords().set(Dim("a_attr"), scalar);

  Dataset d2;
  d2.setData("a", varX);
  d2["a"].coords().set(Dim("a_attr"), varX);
  d2["a"].coords().set(Dim("a_attr2"), varX);

  auto result(d1);

  auto check_preserved_only_lhs_attrs = [&]() {
    ASSERT_EQ(result["a"].coords().size(), 1);
    EXPECT_EQ(result["a"].coords()[Dim("a_attr")], scalar);
  };

  result += d2;
  check_preserved_only_lhs_attrs();
  result -= d2;
  check_preserved_only_lhs_attrs();
  result *= d2;
  check_preserved_only_lhs_attrs();
  result /= d2;
  check_preserved_only_lhs_attrs();
}

TEST_F(AttributesTest, reduction_ops) {
  Dataset d;
  d.setCoord(Dim::X,
             makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 1, 2}));
  d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                      Values{10, 20}));
  d["a"].coords().set(Dim("a_attr"), scalar);
  d["a"].coords().set(Dim("a_attr_x"), varX);

  for (const auto &result :
       {sum(d, Dim::X), mean(d, Dim::X), resize(d, Dim::X, 4),
        rebin(d, Dim::X,
              makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 2}))}) {
    ASSERT_TRUE(result["a"].coords().contains(Dim("a_attr")));
    ASSERT_FALSE(result["a"].coords().contains(Dim("a_attr_x")));
    EXPECT_EQ(result["a"].coords()[Dim("a_attr")], scalar);
  }
}

TEST_F(AttributesTest, scalar_mapped_into_unaligned) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
  d["a"].coords().set(Dim("attr"), scalar);
  EXPECT_TRUE(d["a"].unaligned_coords().contains(Dim("attr")));
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().contains(Dim("attr")));
  EXPECT_THROW(d["a"].unaligned().unaligned_coords().erase(Dim("attr")),
               except::NotFoundError);
  d["a"].coords().erase(Dim("attr"));
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
}

TEST_F(AttributesTest, scalar_not_mapped_into_aligned) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
  EXPECT_THROW(d["a"].unaligned().unaligned_coords().set(Dim("scalar"), scalar),
               except::RealignedDataError);
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
}

TEST_F(AttributesTest, aligned_not_mapped_into_unaligned) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
  d["a"].coords().set(Dim("attr"),
                      makeVariable<double>(Dims{Dim::Y}, Shape{1}));
  EXPECT_TRUE(d["a"].unaligned_coords().contains(Dim("attr")));
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
  EXPECT_THROW(d["a"].unaligned().coords().erase(Dim("attr")),
               except::NotFoundError);
  d["a"].coords().erase(Dim("attr"));
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
}

TEST_F(AttributesTest, unaligned_not_mapped_into_aligned) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
  d["a"].unaligned().unaligned_coords().set(
      Dim("attr"), makeVariable<double>(Dims{Dim::X}, Shape{3}));
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().contains(Dim("attr")));
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
  EXPECT_THROW(d["a"].coords().erase(Dim("attr")), except::NotFoundError);
  d["a"].unaligned().coords().erase(Dim("attr"));
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
}

// We have removed the check in Dataset::setAttr preventing insertion of attrs
// exceeding data dims. This is now more in line with how coords are handled,
// and is required for storing edges of a single bin created from a non-range
// slice. However, it leaves this peculiarity of allowing insertion of an
// attribute that depends on an dimension of unaligned content, without implying
// actual relation, i.e., extents are unrelated.
TEST_F(AttributesTest, DISABLED_unaligned_set_via_aligned_fails) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_ANY_THROW(d["a"].coords().set(
      Dim("x"), makeVariable<double>(Dims{Dim::X}, Shape{3})));
  EXPECT_TRUE(d["a"].unaligned().unaligned_coords().empty());
  EXPECT_TRUE(d["a"].unaligned_coords().empty());
}
