// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include <set>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

template <typename T> class DatasetProxyTest : public ::testing::Test {
protected:
  template <class D> T access(D &dataset) { return dataset; }
};

using DatasetProxyTypes = ::testing::Types<Dataset &, const Dataset &,
                                           DatasetProxy, DatasetConstProxy>;
TYPED_TEST_SUITE(DatasetProxyTest, DatasetProxyTypes);

TYPED_TEST(DatasetProxyTest, empty) {
  Dataset d;
  auto &&proxy = TestFixture::access(d);
  ASSERT_TRUE(proxy.empty());
  ASSERT_EQ(proxy.size(), 0);
}

TYPED_TEST(DatasetProxyTest, coords) {
  Dataset d;
  auto &&proxy = TestFixture::access(d);
  ASSERT_NO_THROW(proxy.coords());
}

TYPED_TEST(DatasetProxyTest, labels) {
  Dataset d;
  auto &&proxy = TestFixture::access(d);
  ASSERT_NO_THROW(proxy.labels());
}

TYPED_TEST(DatasetProxyTest, attrs) {
  Dataset d;
  auto &&proxy = TestFixture::access(d);
  ASSERT_NO_THROW(proxy.attrs());
}

TYPED_TEST(DatasetProxyTest, bad_item_access) {
  Dataset d;
  auto &&proxy = TestFixture::access(d);
  ASSERT_ANY_THROW(proxy[""]);
  ASSERT_ANY_THROW(proxy["abc"]);
}

TYPED_TEST(DatasetProxyTest, name) {
  Dataset d;
  d.setData("a", createVariable<double>(Values{double{}}));
  d.setData("b", createVariable<float>(Values{float{}}));
  d.setData("c", createVariable<int64_t>(Values{int64_t{}}));
  auto &&proxy = TestFixture::access(d);

  for (const auto &name : {"a", "b", "c"})
    EXPECT_EQ(proxy[name].name(), name);
  for (const auto &name : {"a", "b", "c"})
    EXPECT_EQ(proxy.find(name)->second.name(), name);
}

TYPED_TEST(DatasetProxyTest, find_and_contains) {
  Dataset d;
  d.setData("a", createVariable<double>(Values{double{}}));
  d.setData("b", createVariable<float>(Values{float{}}));
  d.setData("c", createVariable<int64_t>(Values{int64_t{}}));
  auto &&proxy = TestFixture::access(d);

  EXPECT_EQ(proxy.find("not a thing"), proxy.end());
  EXPECT_EQ(proxy.find("a")->first, "a");
  EXPECT_EQ(proxy.find("a")->second, proxy["a"]);
  EXPECT_FALSE(proxy.contains("not a thing"));
  EXPECT_TRUE(proxy.contains("a"));

  EXPECT_EQ(proxy.find("b")->first, "b");
  EXPECT_EQ(proxy.find("b")->second, proxy["b"]);
}

TYPED_TEST(DatasetProxyTest, find_in_slice) {
  Dataset d;
  d.setCoord(Dim::X, createVariable<double>(Dims{Dim::X}, Shape{2}));
  d.setCoord(Dim::Y, createVariable<double>(Dims{Dim::Y}, Shape{2}));
  d.setData("a", createVariable<double>(Dims{Dim::X}, Shape{2}));
  d.setData("b", createVariable<float>(Dims{Dim::Y}, Shape{2}));
  auto &&proxy = TestFixture::access(d);

  const auto slice = proxy.slice({Dim::X, 1});

  EXPECT_EQ(slice.find("a")->first, "a");
  EXPECT_EQ(slice.find("a")->second, slice["a"]);
  EXPECT_EQ(slice.find("b"), slice.end());
  EXPECT_TRUE(slice.contains("a"));
  EXPECT_FALSE(slice.contains("b"));
}

TYPED_TEST(DatasetProxyTest, iterators_empty_dataset) {
  Dataset d;
  auto &&proxy = TestFixture::access(d);
  ASSERT_NO_THROW(proxy.begin());
  ASSERT_NO_THROW(proxy.end());
  EXPECT_EQ(proxy.begin(), proxy.end());
}

TYPED_TEST(DatasetProxyTest, iterators_only_coords) {
  Dataset d;
  d.setCoord(Dim::X, createVariable<double>(Values{double{}}));
  auto &&proxy = TestFixture::access(d);
  ASSERT_NO_THROW(proxy.begin());
  ASSERT_NO_THROW(proxy.end());
  EXPECT_EQ(proxy.begin(), proxy.end());
}

TYPED_TEST(DatasetProxyTest, iterators_only_labels) {
  Dataset d;
  d.setLabels("a", createVariable<double>(Values{double{}}));
  auto &&proxy = TestFixture::access(d);
  ASSERT_NO_THROW(proxy.begin());
  ASSERT_NO_THROW(proxy.end());
  EXPECT_EQ(proxy.begin(), proxy.end());
}

TYPED_TEST(DatasetProxyTest, iterators_only_attrs) {
  Dataset d;
  d.setAttr("a", createVariable<double>(Values{double{}}));
  auto &&proxy = TestFixture::access(d);
  ASSERT_NO_THROW(proxy.begin());
  ASSERT_NO_THROW(proxy.end());
  EXPECT_EQ(proxy.begin(), proxy.end());
}

TYPED_TEST(DatasetProxyTest, iterators) {
  Dataset d;
  d.setData("a", createVariable<double>(Values{double{}}));
  d.setData("b", createVariable<float>(Values{float{}}));
  d.setData("c", createVariable<int64_t>(Values{int64_t{}}));
  auto &&proxy = TestFixture::access(d);

  ASSERT_NO_THROW(proxy.begin());
  ASSERT_NO_THROW(proxy.end());

  std::set<std::string> found;
  std::set<std::string> expected{"a", "b", "c"};

  auto it = proxy.begin();
  ASSERT_NE(it, proxy.end());
  found.insert(it->first);

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, proxy.end());
  found.insert(it->first);

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, proxy.end());
  found.insert(it->first);

  EXPECT_EQ(found, expected);

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, proxy.end());
}
