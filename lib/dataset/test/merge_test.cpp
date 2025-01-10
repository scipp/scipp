// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(MergeTest, simple) {
  Dataset a(
      {{"data_1",
        makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{15, 16, 17})}},
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})},
       {Dim("label_1"),
        makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{9, 8, 7})}});

  Dataset b(
      {{"data_2",
        makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13})}},
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})},
       {Dim("label_2"),
        makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{9, 8, 9})}});

  const auto d = merge(a, b);

  EXPECT_EQ(a.coords()[Dim::X], d.coords()[Dim::X]);

  EXPECT_EQ(a["data_1"].data(), d["data_1"].data());
  EXPECT_EQ(b["data_2"].data(), d["data_2"].data());

  EXPECT_EQ(a.coords()[Dim("label_1")], d.coords()[Dim("label_1")]);
  EXPECT_EQ(b.coords()[Dim("label_2")], d.coords()[Dim("label_2")]);
}

TEST(MergeTest, non_matching_dense_data) {
  Dataset a({{"data", makeVariable<int>(Dims{Dim::X}, Shape{5},
                                        Values{1, 2, 3, 4, 5})}});
  Dataset b({{"data", makeVariable<int>(Dims{Dim::X}, Shape{5},
                                        Values{2, 3, 4, 5, 6})}});
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_dense_coords) {
  Dataset a({}, {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{5},
                                            Values{1, 2, 3, 4, 5})}});
  Dataset b({}, {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{5},
                                            Values{2, 3, 4, 5, 6})}});
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}

TEST(MergeTest, non_matching_dense_labels) {
  Dataset a({}, {{Dim("l"), makeVariable<int>(Dims{Dim::X}, Shape{5},
                                              Values{1, 2, 3, 4, 5})}});
  Dataset b({}, {{Dim("l"), makeVariable<int>(Dims{Dim::X}, Shape{5},
                                              Values{2, 3, 4, 5, 6})}});
  EXPECT_THROW(auto d = merge(a, b), std::runtime_error);
}
