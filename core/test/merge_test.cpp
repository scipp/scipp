// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

TEST(MergeTest, simple) {
  Dataset a;
  a.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}, {1, 2, 3}));
  a.setCoord(Dim::Y, makeVariable<int>({Dim::Y, 3}, {6, 7, 8}));
  a.setData("data_1", makeVariable<int>({Dim::X, 3}, {15, 16, 17}));
  a.setLabels("label_1", makeVariable<int>({Dim::Y, 3}, {9, 8, 7}));
  a.setAttr("attr_1", makeVariable<int>(42));
  a.setAttr("attr_2", makeVariable<int>(495));

  Dataset b;
  b.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}, {1, 2, 3}));
  b.setData("data_2", makeVariable<int>({Dim::X, 3}, {11, 12, 13}));
  b.setLabels("label_2", makeVariable<int>({Dim::X, 3}, {9, 8, 9}));
  b.setAttr("attr_2", makeVariable<int>(495));

  const auto d = merge(a, b);

  EXPECT_EQ(a.coords()[Dim::X], d.coords()[Dim::X]);
  EXPECT_EQ(a.coords()[Dim::Y], d.coords()[Dim::Y]);

  EXPECT_EQ(a["data_1"].data(), d["data_1"].data());
  EXPECT_EQ(b["data_2"].data(), d["data_2"].data());

  EXPECT_EQ(a.labels()["label_1"], d.labels()["label_1"]);
  EXPECT_EQ(b.labels()["label_2"], d.labels()["label_2"]);

  EXPECT_EQ(a.attrs()["attr_1"], d.attrs()["attr_1"]);
  EXPECT_EQ(b.attrs()["attr_2"], d.attrs()["attr_2"]);
}

TEST(MergeTest, sparse) {
  auto sparseCoord = makeVariable<int>({Dim::X, Dimensions::Sparse});
  sparseCoord.sparseValues<int>()[0] = {1, 2, 3, 4};

  Dataset a;
  {
    a.setData("sparse", makeVariable<int>({Dim::X, Dimensions::Sparse}));
    a.setSparseCoord("sparse", sparseCoord);
  }

  Dataset b;
  {
    b.setData("sparse", makeVariable<int>({Dim::X, Dimensions::Sparse}));
    b.setSparseCoord("sparse", sparseCoord);
  }

  const auto d = merge(a, b);

  EXPECT_EQ(a["sparse"].data(), d["sparse"].data());
  EXPECT_EQ(b["sparse"].data(), d["sparse"].data());
}

TEST(MergeTest, non_matching_dense_data) {
  Dataset a;
  Dataset b;
  a.setData("data", makeVariable<int>({Dim::X, 5}, {1, 2, 3, 4, 5}));
  b.setData("data", makeVariable<int>({Dim::X, 5}, {2, 3, 4, 5, 6}));
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_sparse_data) {
  Dataset a;
  {
    auto data = makeVariable<int>({Dim::X, Dim::Y}, {1, Dimensions::Sparse});
    data.sparseValues<int>()[0] = {2, 3};
    a.setData("sparse", data);
  }

  Dataset b;
  {
    auto data = makeVariable<int>({Dim::X, Dim::Y}, {1, Dimensions::Sparse});
    data.sparseValues<int>()[0] = {1, 2};
    b.setData("sparse", data);
  }

  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_dense_coords) {
  Dataset a;
  Dataset b;
  a.setCoord(Dim::X, makeVariable<int>({Dim::X, 5}, {1, 2, 3, 4, 5}));
  b.setCoord(Dim::X, makeVariable<int>({Dim::X, 5}, {2, 3, 4, 5, 6}));
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_sparse_coords) {
  Dataset a;
  {
    auto coord = makeVariable<int>({Dim::X, Dim::Y}, {1, Dimensions::Sparse});
    coord.sparseValues<int>()[0] = {2, 3};
    a.setSparseCoord("sparse", coord);
  }

  Dataset b;
  {
    auto coord = makeVariable<int>({Dim::X, Dim::Y}, {1, Dimensions::Sparse});
    coord.sparseValues<int>()[0] = {1, 2};
    b.setSparseCoord("sparse", coord);
  }

  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_dense_labels) {
  Dataset a;
  Dataset b;
  a.setLabels("l", makeVariable<int>({Dim::X, 5}, {1, 2, 3, 4, 5}));
  b.setLabels("l", makeVariable<int>({Dim::X, 5}, {2, 3, 4, 5, 6}));
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_sparse_labels) {
  auto coord = makeVariable<int>({Dim::X, Dim::Y}, {1, Dimensions::Sparse});
  coord.sparseValues<int>()[0] = {1, 2};

  Dataset a;
  {
    auto label = makeVariable<int>({Dim::X, Dim::Y}, {1, Dimensions::Sparse});
    label.sparseValues<int>()[0] = {2, 3};
    a.setSparseCoord("sparse", coord);
    a.setSparseLabels("sparse", "l", label);
  }

  Dataset b;
  {
    auto label = makeVariable<int>({Dim::X, Dim::Y}, {1, Dimensions::Sparse});
    label.sparseValues<int>()[0] = {1, 2};
    b.setSparseCoord("sparse", coord);
    b.setSparseLabels("sparse", "l", label);
  }

  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_attrs) {
  Dataset a;
  Dataset b;
  a.setAttr("a", makeVariable<int>({Dim::X, 5}, {1, 2, 3, 4, 5}));
  b.setAttr("a", makeVariable<int>({Dim::X, 5}, {2, 3, 4, 5, 6}));
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}
