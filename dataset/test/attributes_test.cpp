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

TEST_F(AttributesTest, dataset_attrs) {
  Dataset d;
  d.setAttr("scalar", scalar);
  d.setAttr("x", varX);
  ASSERT_EQ(d.attrs().size(), 2);
  ASSERT_TRUE(d.attrs().contains("scalar"));
  ASSERT_TRUE(d.attrs().contains("x"));
  const auto attrs = d.attrs();
  ASSERT_EQ(std::set<std::string>(attrs.keys_begin(), attrs.keys_end()),
            (std::set<std::string>{"scalar", "x"}));
  ASSERT_EQ(d.dimensions(),
            (std::unordered_map<Dim, scipp::index>{{Dim::X, 2}}));
  d.eraseAttr("scalar");
  d.eraseAttr("x");
  ASSERT_EQ(d.attrs().size(), 0);
  ASSERT_EQ(d.dimensions(), (std::unordered_map<Dim, scipp::index>{}));
}

TEST_F(AttributesTest, dataset_item_attrs) {
  Dataset d;
  d.setData("a", varX);
  d["a"].attrs().set("scalar", scalar);
  d["a"].attrs().set("x", varX);
  d.attrs().set("dataset_attr", scalar);

  ASSERT_FALSE(d.attrs().contains("scalar"));
  ASSERT_FALSE(d.attrs().contains("x"));

  ASSERT_EQ(d["a"].attrs().size(), 2);
  ASSERT_TRUE(d["a"].attrs().contains("scalar"));
  ASSERT_TRUE(d["a"].attrs().contains("x"));
  ASSERT_FALSE(d["a"].attrs().contains("dataset_attr"));

  d["a"].attrs().erase("scalar");
  d["a"].attrs().erase("x");
  ASSERT_EQ(d["a"].attrs().size(), 0);
}

TEST_F(AttributesTest, dataset_events_item_attrs) {
  Dataset d;
  d.setData("events", makeVariable<event_list<double>>(Dims{}, Shape{}));
  d["events"].attrs().set("scalar", scalar);
  d.attrs().set("dataset_attr", scalar);

  ASSERT_FALSE(d.attrs().contains("scalar"));

  ASSERT_EQ(d["events"].attrs().size(), 1);
  ASSERT_TRUE(d["events"].attrs().contains("scalar"));
  ASSERT_FALSE(d["events"].attrs().contains("dataset_attr"));

  d["events"].attrs().erase("scalar");
  ASSERT_EQ(d["events"].attrs().size(), 0);
}

TEST_F(AttributesTest, slice_dataset_item_attrs) {
  Dataset d;
  d.setData("a", varZX);
  d["a"].attrs().set("scalar", scalar);
  d["a"].attrs().set("x", varX);

  // Same behavior as coord slicing:
  // - Lower-dimensional attrs are not hidden by slicing.
  // - Non-range slice hides attribute.
  // The alternative would be to handle attributes like data, but at least for
  // now coord-like handling appears to make more sense.
  ASSERT_TRUE(d["a"].slice({Dim::X, 0}).attrs().contains("scalar"));
  ASSERT_FALSE(d["a"].slice({Dim::X, 0}).attrs().contains("x"));
  ASSERT_TRUE(d["a"].slice({Dim::X, 0, 1}).attrs().contains("scalar"));
  ASSERT_TRUE(d["a"].slice({Dim::X, 0, 1}).attrs().contains("x"));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0}).attrs().contains("scalar"));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0}).attrs().contains("x"));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0, 1}).attrs().contains("scalar"));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0, 1}).attrs().contains("x"));
}

TEST_F(AttributesTest, binary_ops_matching_attrs_preserved) {
  Dataset d;
  d.setData("a", varX);
  d["a"].attrs().set("a_attr", scalar);
  d.attrs().set("dataset_attr", scalar);

  for (const auto &result : {d + d, d - d, d * d, d / d}) {
    EXPECT_EQ(result.attrs(), d.attrs());
    EXPECT_EQ(result["a"].attrs(), d["a"].attrs());
  }
}

TEST_F(AttributesTest, binary_ops_mismatching_attrs_dropped) {
  Dataset d1;
  d1.setData("a", varX);
  d1["a"].attrs().set("a_attr", scalar);
  d1.attrs().set("dataset_attr", scalar);
  Dataset d2;
  d2.setData("a", varX);
  d2["a"].attrs().set("a_attr", scalar + scalar); // mismatching content
  d2.attrs().set("dataset_attr", scalar + scalar);
  d2["a"].attrs().set("a_attr2", scalar); // mismatching name
  d2.attrs().set("dataset_attr2", scalar);

  for (const auto &result : {d1 + d2, d1 - d2, d1 * d2, d1 / d2}) {
    EXPECT_TRUE(result.attrs().empty());
    EXPECT_TRUE(result["a"].attrs().empty());
  }
}

TEST_F(AttributesTest, binary_ops_in_place) {
  Dataset d1;
  d1.setData("a", varX);
  d1["a"].attrs().set("a_attr", scalar);
  d1.attrs().set("dataset_attr", scalar);

  Dataset d2;
  d2.setData("a", varX);
  d2["a"].attrs().set("a_attr", varX);
  d2["a"].attrs().set("a_attr2", varX);
  d2.attrs().set("dataset_attr", varX);
  d2.attrs().set("dataset_attr2", varX);

  auto result(d1);

  auto check_preserved_only_lhs_attrs = [&]() {
    ASSERT_EQ(result.attrs().size(), 1);
    EXPECT_EQ(result.attrs()["dataset_attr"], scalar);
    ASSERT_EQ(result["a"].attrs().size(), 1);
    EXPECT_EQ(result["a"].attrs()["a_attr"], scalar);
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
  d["a"].attrs().set("a_attr", scalar);
  d["a"].attrs().set("a_attr_x", varX);
  d.attrs().set("dataset_attr", scalar);
  d.attrs().set("dataset_attr_x", varX);

  for (const auto &result :
       {sum(d, Dim::X), mean(d, Dim::X), resize(d, Dim::X, 4),
        rebin(d, Dim::X,
              makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 2}))}) {
    ASSERT_TRUE(result.attrs().contains("dataset_attr"));
    ASSERT_FALSE(result.attrs().contains("dataset_attr_x"));
    EXPECT_EQ(result.attrs()["dataset_attr"], scalar);
    ASSERT_TRUE(result["a"].attrs().contains("a_attr"));
    ASSERT_FALSE(result["a"].attrs().contains("a_attr_x"));
    EXPECT_EQ(result["a"].attrs()["a_attr"], scalar);
  }
}

TEST_F(AttributesTest, scalar_mapped_into_unaligned) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
  d["a"].attrs().set("scalar", scalar);
  EXPECT_TRUE(d["a"].attrs().contains("scalar"));
  EXPECT_TRUE(d["a"].unaligned().attrs().contains("scalar"));
  EXPECT_THROW(d["a"].unaligned().attrs().erase("scalar"),
               except::NotFoundError);
  d["a"].attrs().erase("scalar");
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
}

TEST_F(AttributesTest, scalar_not_mapped_into_aligned) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
  d["a"].unaligned().attrs().set("scalar", scalar);
  // Note that based on dimensionality we *could* insert this attribute directly
  // in item "a", but it would be confusing if it suddenly appeared on a higher
  // level.
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().contains("scalar"));
  d["a"].unaligned().attrs().erase("scalar");
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
}

TEST_F(AttributesTest, aligned_not_mapped_into_unaligned) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
  d["a"].attrs().set("y", makeVariable<double>(Dims{Dim::Y}, Shape{1}));
  EXPECT_TRUE(d["a"].attrs().contains("y"));
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
  EXPECT_THROW(d["a"].unaligned().attrs().erase("y"), except::NotFoundError);
  d["a"].attrs().erase("y");
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
}

TEST_F(AttributesTest, unaligned_not_mapped_into_aligned) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
  d["a"].unaligned().attrs().set("x",
                                 makeVariable<double>(Dims{Dim::X}, Shape{3}));
  EXPECT_TRUE(d["a"].unaligned().attrs().contains("x"));
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_THROW(d["a"].attrs().erase("x"), except::NotFoundError);
  d["a"].unaligned().attrs().erase("x");
  EXPECT_TRUE(d["a"].attrs().empty());
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
}

// We have removed the check in Dataset::setAttr preventing insertion of attrs
// exceeding data dims. This is now more in line with how coords are handled,
// and is required for storing edges of a single bin created from a non-range
// slice. However, it leaves this peculiarity of allowing insertion of an
// attribute that depends on an dimension of unaligned content, without implying
// actual relation, i.e., extents are unrelated.
TEST_F(AttributesTest, DISABLED_unaligned_set_via_aligned_fails) {
  auto d = testdata::make_dataset_realigned_x_to_y();
  EXPECT_ANY_THROW(
      d["a"].attrs().set("x", makeVariable<double>(Dims{Dim::X}, Shape{3})));
  EXPECT_TRUE(d["a"].unaligned().attrs().empty());
  EXPECT_TRUE(d["a"].attrs().empty());
}
