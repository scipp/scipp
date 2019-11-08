// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;
  // to test:
  // - insert erase dataset
  // - insert data with attr
  // - insert erase item attr
  // - operations with dataset and item attrs (sum, or directly apply_to_items?)
  // - binary in-place operations preserving dataset and item attrs
  // - slicing

class AttributesTest : public ::testing::Test {
protected:
  const Variable scalar = makeVariable<double>(1.0);
  const Variable var1d = makeVariable<double>({Dim::X, 2}, {2.0, 3.0});
};

TEST_F(AttributesTest, dataset_attrs) {
  Dataset d;
  d.setAttr("scalar", scalar);
  d.setAttr("1d", var1d);
  ASSERT_EQ(d.attrs().size(), 2);
  ASSERT_TRUE(d.attrs().contains("scalar"));
  ASSERT_TRUE(d.attrs().contains("1d"));
  ASSERT_EQ(d.dimensions(),
            (std::unordered_map<Dim, scipp::index>{{Dim::X, 2}}));
  d.eraseAttr("scalar");
  d.eraseAttr("1d");
  ASSERT_EQ(d.attrs().size(), 0);
  ASSERT_EQ(d.dimensions(), (std::unordered_map<Dim, scipp::index>{}));
}

TEST_F(AttributesTest, dataset_item_attrs) {
  Dataset d;
  d.setData("a", var1d);
  d["a"].attrs().set("scalar", scalar);
  d["a"].attrs().set("1d", var1d);
  d.attrs().set("dataset_attr", scalar);

  ASSERT_FALSE(d.attrs().contains("scalar"));
  ASSERT_FALSE(d.attrs().contains("1d"));

  ASSERT_EQ(d["a"].attrs().size(), 2);
  ASSERT_TRUE(d["a"].attrs().contains("scalar"));
  ASSERT_TRUE(d["a"].attrs().contains("1d"));
  ASSERT_FALSE(d["a"].attrs().contains("dataset_attr"));

  d["a"].attrs().erase("scalar");
  d["a"].attrs().erase("1d");
  ASSERT_EQ(d["a"].attrs().size(), 0);
}

TEST_F(AttributesTest, dataset_item_attrs_dimensions_exceeding_data) {
  Dataset d;
  d.setData("scalar", scalar);
  EXPECT_THROW(d["scalar"].attrs().set("1d", var1d), except::DimensionError);
}
