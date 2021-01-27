// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(MergeTest, simple) {
  Dataset a;
  a.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  a.setCoord(Dim::Y,
             makeVariable<int>(Dims{Dim::Y}, Shape{3}, Values{6, 7, 8}));
  a.setData("data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{15, 16, 17}));
  a.setCoord(Dim("label_1"),
             makeVariable<int>(Dims{Dim::Y}, Shape{3}, Values{9, 8, 7}));
  a["data_1"].attrs().set(Dim("attr_1"), makeVariable<int>(Values{42}));
  a["data_1"].attrs().set(Dim("attr_2"), makeVariable<int>(Values{495}));

  Dataset b;
  b.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  b.setData("data_2",
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13}));
  b.setCoord(Dim("label_2"),
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{9, 8, 9}));
  b["data_2"].attrs().set(Dim("attr_2"), makeVariable<int>(Values{495}));

  const auto d = merge(a, b);

  EXPECT_EQ(a.coords()[Dim::X], d.coords()[Dim::X]);
  EXPECT_EQ(a.coords()[Dim::Y], d.coords()[Dim::Y]);

  EXPECT_EQ(a["data_1"].data(), d["data_1"].data());
  EXPECT_EQ(b["data_2"].data(), d["data_2"].data());

  EXPECT_EQ(a.coords()[Dim("label_1")], d.coords()[Dim("label_1")]);
  EXPECT_EQ(b.coords()[Dim("label_2")], d.coords()[Dim("label_2")]);

  EXPECT_EQ(a["data_1"].attrs()[Dim("attr_1")],
            d["data_1"].attrs()[Dim("attr_1")]);
  EXPECT_EQ(b["data_2"].attrs()[Dim("attr_2")],
            d["data_2"].attrs()[Dim("attr_2")]);
}

TEST(MergeTest, non_matching_dense_data) {
  Dataset a;
  Dataset b;
  a.setData("data",
            makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{1, 2, 3, 4, 5}));
  b.setData("data",
            makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{2, 3, 4, 5, 6}));
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_dense_coords) {
  Dataset a;
  Dataset b;
  a.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{1, 2, 3, 4, 5}));
  b.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{2, 3, 4, 5, 6}));
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_dense_labels) {
  Dataset a;
  Dataset b;
  a.setCoord(Dim("l"),
             makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{1, 2, 3, 4, 5}));
  b.setCoord(Dim("l"),
             makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{2, 3, 4, 5, 6}));
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}
