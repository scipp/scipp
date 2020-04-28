// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <limits>
#include <set>
#include <thread>

#include "scipp/common/index.h"
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
  const auto expected = static_cast<int64_t>(Dim::Id::Invalid);
  for (int64_t i = 0; i < expected; ++i)
    names.emplace(Dim(static_cast<Dim::Id>(i)).name());
  EXPECT_EQ(names.size(), expected);
}

void add_dims() {
  for (int64_t i = 0; i < 128; ++i)
    for (scipp::index repeat = 0; repeat < 16; ++repeat)
      static_cast<void>(Dim("custom" + std::to_string(i)).name());
}

TEST(DimTest, thread_safe) {
  std::vector<std::thread> threads;
  threads.reserve(100);
  for (auto i = 0; i < 100; ++i)
    threads.emplace_back(add_dims);
  for (auto &t : threads)
    t.join();
}

// This tests works, but is in conflict with the thread-safety test, since there
// is no way of resetting the static map of known custom labels. It can be run
// separately though.
TEST(DimTest, DISABLED_label_count_overflow) {
  // Note that the id of "first" is not necessarily the value coded in the
  // implementation but rather depends on which tests have run before.
  const auto end =
      std::numeric_limits<std::underlying_type<Dim::Id>::type>::max();
  const auto count = end - static_cast<int64_t>(Dim("first").id());
  for (int64_t i = 0; i < count; ++i)
    Dim("custom" + std::to_string(i));
  EXPECT_EQ(
      static_cast<int64_t>(Dim("custom" + std::to_string(count - 1)).id()),
      end);
  EXPECT_THROW(Dim("overflow"), std::runtime_error);
}
