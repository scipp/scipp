// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>
#include <set>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

TEST(DatasetTest, construct_default) { ASSERT_NO_THROW(Dataset d); }

TEST(DatasetTest, empty) {
  Dataset d;
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.size(), 0);
}

TEST(DatasetTest, coords) {
  Dataset d;
  ASSERT_NO_THROW(d.coords());
}

TEST(DatasetTest, labels) {
  Dataset d;
  ASSERT_NO_THROW(d.labels());
}

TEST(DatasetTest, attrs) {
  Dataset d;
  ASSERT_NO_THROW(d.attrs());
}

TEST(DatasetTest, bad_item_access) {
  Dataset d;
  ASSERT_ANY_THROW(d[""]);
  ASSERT_ANY_THROW(d["abc"]);
}

TEST(DatasetTest, setCoord) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 0);

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 1);

  ASSERT_NO_THROW(d.setCoord(Dim::Y, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 2);

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 2);
}

TEST(DatasetTest, setLabels) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 0);

  ASSERT_NO_THROW(d.setLabels("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 1);

  ASSERT_NO_THROW(d.setLabels("b", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 2);

  ASSERT_NO_THROW(d.setLabels("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 2);
}

TEST(DatasetTest, setAttr) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 0);

  ASSERT_NO_THROW(d.setAttr("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 1);

  ASSERT_NO_THROW(d.setAttr("b", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 2);

  ASSERT_NO_THROW(d.setAttr("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 2);
}

TEST(DatasetTest, setData_with_and_without_variances) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_NO_THROW(d.setData("a", var));
  ASSERT_EQ(d.size(), 1);

  ASSERT_NO_THROW(d.setData("b", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setData("a", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(
      d.setData("a", makeVariable<double>({Dim::X, 3}, {1, 1, 1}, {0, 0, 0})));
  ASSERT_EQ(d.size(), 2);
}

TEST(DatasetTest, setLabels_with_name_matching_data_name) {
  Dataset d;
  d.setData("a", makeVariable<double>({Dim::X, 3}));
  d.setData("b", makeVariable<double>({Dim::X, 3}));

  // It is possible to set labels with a name matching data. However, there is
  // no special meaning attached to this. In particular it is *not* linking the
  // labels to that data item.
  ASSERT_NO_THROW(d.setLabels("a", makeVariable<double>({})));
  ASSERT_EQ(d.size(), 2);
  ASSERT_EQ(d.labels().size(), 1);
  ASSERT_EQ(d["a"].labels().size(), 1);
  ASSERT_EQ(d["b"].labels().size(), 1);
}

TEST(DatasetTest, setSparseCoord_not_sparse_fail) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_ANY_THROW(d.setSparseCoord("a", var));
}

TEST(DatasetTest, setSparseCoord) {
  Dataset d;
  const auto var =
      makeVariable<double>({Dim::X, Dim::Y}, {3, Dimensions::Sparse});

  ASSERT_NO_THROW(d.setSparseCoord("a", var));
  ASSERT_EQ(d.size(), 1);
  ASSERT_NO_THROW(d["a"]);
}

TEST(DatasetTest, setSparseLabels_missing_values_or_coord) {
  Dataset d;
  const auto sparse = makeVariable<double>({Dim::X}, {Dimensions::Sparse});

  ASSERT_ANY_THROW(d.setSparseLabels("a", "x", sparse));
  d.setSparseCoord("a", sparse);
  ASSERT_NO_THROW(d.setSparseLabels("a", "x", sparse));
}

TEST(DatasetTest, setSparseLabels_not_sparse_fail) {
  Dataset d;
  const auto dense = makeVariable<double>({});
  const auto sparse = makeVariable<double>({Dim::X}, {Dimensions::Sparse});

  d.setSparseCoord("a", sparse);
  ASSERT_ANY_THROW(d.setSparseLabels("a", "x", dense));
}

TEST(DatasetTest, setSparseLabels) {
  Dataset d;
  const auto sparse = makeVariable<double>({Dim::X}, {Dimensions::Sparse});
  d.setSparseCoord("a", sparse);

  ASSERT_NO_THROW(d.setSparseLabels("a", "x", sparse));
  ASSERT_EQ(d.size(), 1);
  ASSERT_NO_THROW(d["a"]);
  ASSERT_EQ(d["a"].labels().size(), 1);
}

TEST(DatasetTest, iterators_empty_dataset) {
  Dataset d;
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetTest, iterators_only_coords) {
  Dataset d;
  d.setCoord(Dim::X, makeVariable<double>({}));
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetTest, iterators_only_labels) {
  Dataset d;
  d.setLabels("a", makeVariable<double>({}));
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetTest, iterators_only_attrs) {
  Dataset d;
  d.setAttr("a", makeVariable<double>({}));
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetTest, iterators) {
  Dataset d;
  d.setData("a", makeVariable<double>({}));
  d.setData("b", makeVariable<float>({}));
  d.setData("c", makeVariable<int64_t>({}));

  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());

  std::set<std::string> found;
  std::set<std::string> expected{"a", "b", "c"};

  auto it = d.begin();
  ASSERT_NE(it, d.end());
  found.insert(it->first);

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, d.end());
  found.insert(it->first);

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, d.end());
  found.insert(it->first);

  EXPECT_EQ(found, expected);

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, d.end());
}

TEST(DatasetTest, iterators_return_types) {
  Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(d.begin()->second), DataProxy>));
  ASSERT_TRUE((std::is_same_v<decltype(d.end()->second), DataProxy>));
}

TEST(DatasetTest, const_iterators_return_types) {
  const Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(d.begin()->second), DataConstProxy>));
  ASSERT_TRUE((std::is_same_v<decltype(d.end()->second), DataConstProxy>));
}

TEST(DatasetTest, set_dense_data_with_sparse_coord) {
  auto sparse_variable =
      makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto dense_variable = makeVariable<double>({Dim::Y, Dim::X}, {2, 2});

  Dataset a;
  a.setData("sparse_coord_and_val", dense_variable);
  ASSERT_THROW(a.setSparseCoord("sparse_coord_and_val", sparse_variable),
               except::DimensionError);

  // Setting coords first yields same response.
  Dataset b;
  b.setSparseCoord("sparse_coord_and_val", sparse_variable);
  ASSERT_THROW(b.setData("sparse_coord_and_val", dense_variable),
               except::DimensionError);
}

TEST(DatasetTest, construct_from_proxy) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();
  const DatasetConstProxy proxy(dataset);
  Dataset from_proxy(proxy);
  ASSERT_EQ(from_proxy, dataset);
}

TEST(DatasetTest, construct_from_slice) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();
  const auto slice = dataset.slice({Dim::X, 1});
  Dataset from_slice(slice);
  ASSERT_EQ(from_slice, dataset.slice({Dim::X, 1}));
}

TEST(DatasetTest, slice_temporary) {
  DatasetFactory3D factory;
  auto dataset = factory.make().slice({Dim::X, 1});
  ASSERT_TRUE((std::is_same_v<decltype(dataset), Dataset>));
}

template <typename T> class DatasetProxyTest : public ::testing::Test {
protected:
  template <class D>
  std::conditional_t<std::is_same_v<T, Dataset>, Dataset,
                     std::conditional_t<std::is_same_v<T, DatasetProxy>,
                                        DatasetProxy, DatasetConstProxy>>
  access(D &dataset) {
    return dataset;
  }
};

using DatasetProxyTypes =
    ::testing::Types<Dataset, DatasetProxy, DatasetConstProxy>;
TYPED_TEST_SUITE(DatasetProxyTest, DatasetProxyTypes);

TYPED_TEST(DatasetProxyTest, find_and_contains) {
  Dataset d;
  d.setData("a", makeVariable<double>({}));
  d.setData("b", makeVariable<float>({}));
  d.setData("c", makeVariable<int64_t>({}));
  auto proxy = TestFixture::access(d);

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
  d.setCoord(Dim::X, makeVariable<double>({Dim::X, 2}));
  d.setCoord(Dim::Y, makeVariable<double>({Dim::Y, 2}));
  d.setData("a", makeVariable<double>({Dim::X, 2}));
  d.setData("b", makeVariable<float>({Dim::Y, 2}));
  auto proxy = TestFixture::access(d);

  const auto slice = proxy.slice({Dim::X, 1});

  EXPECT_EQ(slice.find("a")->first, "a");
  EXPECT_EQ(slice.find("a")->second, slice["a"]);
  EXPECT_EQ(slice.find("b"), slice.end());
  EXPECT_TRUE(slice.contains("a"));
  EXPECT_FALSE(slice.contains("b"));
}
