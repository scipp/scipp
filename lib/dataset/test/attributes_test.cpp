// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/mean.h"
#include "scipp/dataset/rebin.h"
#include "scipp/dataset/shape.h"
#include "scipp/dataset/sum.h"
#include "scipp/variable/arithmetic.h"

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
  Dataset d({{"a", varX}});
  d["a"].attrs().set(Dim("scalar"), scalar);
  d["a"].attrs().set(Dim("x"), varX);
  d.coords().set(Dim("dataset_attr"), scalar);

  ASSERT_FALSE(d.coords().contains(Dim("scalar")));
  ASSERT_FALSE(d.coords().contains(Dim("x")));

  ASSERT_EQ(d["a"].attrs().size(), 2);
  ASSERT_TRUE(d["a"].attrs().contains(Dim("scalar")));
  ASSERT_TRUE(d["a"].attrs().contains(Dim("x")));
  ASSERT_TRUE(d["a"].coords().contains(Dim("dataset_attr")));
  ASSERT_FALSE(d["a"].attrs().contains(Dim("dataset_attr")));

  d["a"].attrs().erase(Dim("scalar"));
  d["a"].attrs().erase(Dim("x"));
  ASSERT_EQ(d["a"].attrs().size(), 0);
}

TEST_F(AttributesTest, slice_dataset_item_attrs) {
  Dataset d({{"a", varZX}});
  d["a"].attrs().set(Dim("scalar"), scalar);
  d["a"].attrs().set(Dim("x"), varX);

  // Same behavior as coord slicing:
  // - Lower-dimensional attrs are not hidden by slicing.
  // - Non-range slice hides attribute.
  // The alternative would be to handle attributes like data, but at least for
  // now coord-like handling appears to make more sense.
  ASSERT_TRUE(d["a"].slice({Dim::X, 0}).meta().contains(Dim("scalar")));
  ASSERT_FALSE(d["a"].slice({Dim::X, 0}).coords().contains(Dim("x")));
  ASSERT_TRUE(d["a"].slice({Dim::X, 0}).attrs().contains(Dim("x")));
  ASSERT_TRUE(d["a"].slice({Dim::X, 0, 1}).attrs().contains(Dim("scalar")));
  ASSERT_TRUE(d["a"].slice({Dim::X, 0, 1}).attrs().contains(Dim("x")));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0}).attrs().contains(Dim("scalar")));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0}).attrs().contains(Dim("x")));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0, 1}).attrs().contains(Dim("scalar")));
  ASSERT_TRUE(d["a"].slice({Dim::Y, 0, 1}).attrs().contains(Dim("x")));
}

TEST_F(AttributesTest, coords_are_not_transferred_to_attrs_in_slicing) {
  Dataset d({{"a", copy(varX)}});
  d.coords().set(Dim::X, copy(varX));
  ASSERT_TRUE(d.slice({Dim::X, 0})["a"].coords().contains(Dim::X));
  ASSERT_TRUE(d.slice({Dim::X, 0})["a"].coords()[Dim::X].is_readonly());
  ASSERT_FALSE(d.slice({Dim::X, 0})["a"].attrs().contains(Dim::X));
  ASSERT_TRUE(d.slice({Dim::X, 0, 1})["a"].coords().contains(Dim::X));
  ASSERT_TRUE(d.slice({Dim::X, 0, 1})["a"].coords()[Dim::X].is_readonly());
  ASSERT_FALSE(d.slice({Dim::X, 0, 1})["a"].attrs().contains(Dim::X));
}

TEST_F(AttributesTest, binary_ops_matching_attrs_preserved) {
  Dataset d({{"a", varX}});
  d["a"].attrs().set(Dim("a_attr"), scalar);

  for (const auto &result : {d + d, d - d, d * d, d / d}) {
    EXPECT_EQ(result["a"].coords(), d["a"].coords());
  }
}

TEST_F(AttributesTest, binary_ops_mismatching_attrs_dropped) {
  Dataset d1({{"a", varX}});
  d1["a"].attrs().set(Dim("a_attr"), scalar);
  Dataset d2({{"a", varX}});
  d2["a"].attrs().set(Dim("a_attr"), scalar + scalar); // mismatching content
  d2["a"].attrs().set(Dim("a_attr2"), scalar);         // mismatching name

  for (const auto &result : {d1 + d2, d1 - d2, d1 * d2, d1 / d2}) {
    EXPECT_TRUE(result["a"].coords().empty());
  }
}

TEST_F(AttributesTest, binary_ops_in_place) {
  Dataset d1({{"a", varX}});
  d1["a"].attrs().set(Dim("a_attr"), scalar);

  Dataset d2({{"a", varX}});
  d2["a"].attrs().set(Dim("a_attr"), varX);
  d2["a"].attrs().set(Dim("a_attr2"), varX);

  auto result(d1);

  auto check_preserved_only_lhs_attrs = [&]() {
    ASSERT_EQ(result["a"].attrs().size(), 1);
    EXPECT_EQ(result["a"].attrs()[Dim("a_attr")], scalar);
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
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                        Values{10, 20})}});
  d.setCoord(Dim::X,
             makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 1, 2}));
  d["a"].attrs().set(Dim("a_attr"), scalar);
  d["a"].attrs().set(Dim("a_attr_x"), varX);

  for (const auto &result :
       {sum(d, Dim::X), mean(d, Dim::X), resize(d, Dim::X, 4),
        rebin(d, Dim::X,
              makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 2}))}) {
    ASSERT_TRUE(result["a"].attrs().contains(Dim("a_attr")));
    ASSERT_FALSE(result["a"].attrs().contains(Dim("a_attr_x")));
    EXPECT_EQ(result["a"].attrs()[Dim("a_attr")], scalar);
  }
}
