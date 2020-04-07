// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <set>

#include "scipp/dataset/dataset.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

template <typename T> class DatasetViewTest : public ::testing::Test {
protected:
  template <class D> T access(D &dataset) { return dataset; }
};

using DatasetViewTypes =
    ::testing::Types<Dataset &, const Dataset &, DatasetView, DatasetConstView>;
TYPED_TEST_SUITE(DatasetViewTest, DatasetViewTypes);

TYPED_TEST(DatasetViewTest, empty) {
  Dataset d;
  auto &&view = TestFixture::access(d);
  ASSERT_TRUE(view.empty());
  ASSERT_EQ(view.size(), 0);
}

TYPED_TEST(DatasetViewTest, coords) {
  Dataset d;
  auto &&view = TestFixture::access(d);
  ASSERT_NO_THROW(view.coords());
}

TYPED_TEST(DatasetViewTest, attrs) {
  Dataset d;
  auto &&view = TestFixture::access(d);
  ASSERT_NO_THROW(view.attrs());
}

TYPED_TEST(DatasetViewTest, bad_item_access) {
  Dataset d;
  auto &&view = TestFixture::access(d);
  ASSERT_ANY_THROW(view[""]);
  ASSERT_ANY_THROW(view["abc"]);
}

TYPED_TEST(DatasetViewTest, name) {
  Dataset d;
  d.setData("a", makeVariable<double>(Values{double{}}));
  d.setData("b", makeVariable<float>(Values{float{}}));
  d.setData("c", makeVariable<int64_t>(Values{int64_t{}}));
  auto &&view = TestFixture::access(d);

  for (const auto &name : {"a", "b", "c"})
    EXPECT_EQ(view[name].name(), name);
  for (const auto &name : {"a", "b", "c"})
    EXPECT_EQ(view.find(name)->name(), name);
}

TYPED_TEST(DatasetViewTest, find_and_contains) {
  Dataset d;
  d.setData("a", makeVariable<double>(Values{double{}}));
  d.setData("b", makeVariable<float>(Values{float{}}));
  d.setData("c", makeVariable<int64_t>(Values{int64_t{}}));
  auto &&view = TestFixture::access(d);

  EXPECT_EQ(view.find("not a thing"), view.end());
  EXPECT_EQ(view.find("a")->name(), "a");
  EXPECT_EQ(*view.find("a"), view["a"]);
  EXPECT_FALSE(view.contains("not a thing"));
  EXPECT_TRUE(view.contains("a"));

  EXPECT_EQ(view.find("b")->name(), "b");
  EXPECT_EQ(*view.find("b"), view["b"]);
}

TYPED_TEST(DatasetViewTest, find_in_slice) {
  Dataset d;
  d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{2}));
  d.setCoord(Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{2}));
  d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{2}));
  d.setData("b", makeVariable<float>(Dims{Dim::Y}, Shape{2}));
  auto &&view = TestFixture::access(d);

  const auto slice = view.slice({Dim::X, 1});

  EXPECT_EQ(slice.find("a")->name(), "a");
  EXPECT_EQ(*slice.find("a"), slice["a"]);
  EXPECT_EQ(slice.find("b"), slice.end());
  EXPECT_TRUE(slice.contains("a"));
  EXPECT_FALSE(slice.contains("b"));
}

TYPED_TEST(DatasetViewTest, iterators_empty_dataset) {
  Dataset d;
  auto &&view = TestFixture::access(d);
  ASSERT_NO_THROW(view.begin());
  ASSERT_NO_THROW(view.end());
  EXPECT_EQ(view.begin(), view.end());
}

TYPED_TEST(DatasetViewTest, iterators_only_coords) {
  Dataset d;
  d.setCoord(Dim::X, makeVariable<double>(Values{double{}}));
  auto &&view = TestFixture::access(d);
  ASSERT_NO_THROW(view.begin());
  ASSERT_NO_THROW(view.end());
  EXPECT_EQ(view.begin(), view.end());
}

TYPED_TEST(DatasetViewTest, iterators_only_labels) {
  Dataset d;
  d.setCoord(Dim("a"), makeVariable<double>(Values{double{}}));
  auto &&view = TestFixture::access(d);
  ASSERT_NO_THROW(view.begin());
  ASSERT_NO_THROW(view.end());
  EXPECT_EQ(view.begin(), view.end());
}

TYPED_TEST(DatasetViewTest, iterators_only_attrs) {
  Dataset d;
  d.setAttr("a", makeVariable<double>(Values{double{}}));
  auto &&view = TestFixture::access(d);
  ASSERT_NO_THROW(view.begin());
  ASSERT_NO_THROW(view.end());
  EXPECT_EQ(view.begin(), view.end());
}

TYPED_TEST(DatasetViewTest, iterators) {
  Dataset d;
  d.setData("a", makeVariable<double>(Values{double{}}));
  d.setData("b", makeVariable<float>(Values{float{}}));
  d.setData("c", makeVariable<int64_t>(Values{int64_t{}}));
  auto &&view = TestFixture::access(d);

  ASSERT_NO_THROW(view.begin());
  ASSERT_NO_THROW(view.end());

  std::set<std::string> found;
  std::set<std::string> expected{"a", "b", "c"};

  auto it = view.begin();
  ASSERT_NE(it, view.end());
  found.insert(it->name());

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, view.end());
  found.insert(it->name());

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, view.end());
  found.insert(it->name());

  EXPECT_EQ(found, expected);

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, view.end());
}
