// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <set>

#include "scipp/units/dim.h"

using namespace scipp::units;

TEST(DimTest, basics) {
  EXPECT_EQ(Dim(), Dim(Dim::Invalid));
  EXPECT_EQ(Dim(Dim::X), Dim(Dim::X));
  EXPECT_NE(Dim(Dim::X), Dim(Dim::Y));
  EXPECT_EQ(Dim("abc"), Dim("abc"));
  EXPECT_NE(Dim("abc"), Dim("def"));
  EXPECT_EQ(Dim(Dim::X).name(), "x");
  EXPECT_EQ(Dim("abc").name(), "abc");
}

TEST(DimTest, builtin_from_string) { EXPECT_EQ(Dim(Dim::X), Dim("x")); }

TEST(DimTest, id) {
  const auto max_builtin = Dim(Dim::Invalid).id();
  const auto first_custom = Dim("a").id();
  EXPECT_TRUE(max_builtin < first_custom);
  const auto base = static_cast<int64_t>(first_custom);
  EXPECT_EQ(static_cast<int64_t>(Dim("b").id()), base + 1);
  EXPECT_EQ(static_cast<int64_t>(Dim("c").id()), base + 2);
  EXPECT_EQ(static_cast<int64_t>(Dim("a").id()), base);
}

TEST(DimTest, unique_builtin_name) {
  std::set<std::string> names;
  const auto expected = static_cast<int64_t>(DimId::Invalid);
  for (int64_t i = 0; i < expected; ++i)
    names.emplace(Dim(static_cast<DimId>(i)).name());
  EXPECT_EQ(names.size(), expected);
}
