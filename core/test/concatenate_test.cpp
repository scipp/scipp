// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

TEST(ConcatenateTest, simple_1d) {
  Dataset a;
  a.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}, {1, 2, 3}));
  a.setData("data_1", makeVariable<int>({Dim::X, 3}, {11, 12, 13}));
  a.setLabels("label_1", makeVariable<int>({Dim::X, 3}, {21, 22, 23}));
  a.setMasks("mask_1", makeVariable<bool>({Dim::X, 3}, {false, true, false}));

  Dataset b;
  b.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}, {4, 5, 6}));
  b.setData("data_1", makeVariable<int>({Dim::X, 3}, {14, 15, 16}));
  b.setLabels("label_1", makeVariable<int>({Dim::X, 3}, {24, 25, 26}));
  b.setMasks("mask_1", makeVariable<bool>({Dim::X, 3}, {false, true, false}));

  const auto d = concatenate(a, b, Dim::X);

  EXPECT_EQ(d.coords()[Dim::X],
            makeVariable<int>({Dim::X, 6}, {1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(d["data_1"].data(),
            makeVariable<int>({Dim::X, 6}, {11, 12, 13, 14, 15, 16}));
  EXPECT_EQ(d.labels()["label_1"],
            makeVariable<int>({Dim::X, 6}, {21, 22, 23, 24, 25, 26}));
  EXPECT_EQ(d.masks()["mask_1"],
            makeVariable<bool>({Dim::X, 6},
                               {false, true, false, false, true, false}));
}

TEST(ConcatenateTest, simple_1d_histogram) {
  Dataset a;
  a.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}, {1, 2, 3}));
  a.setData("data_1", makeVariable<int>({Dim::X, 2}, {11, 12}));
  a.setLabels("edge_labels", makeVariable<int>({Dim::X, 3}, {21, 22, 23}));
  a.setLabels("labels", makeVariable<int>({Dim::X, 2}, {21, 22}));
  a.setMasks("masks", makeVariable<bool>({Dim::X, 2}, {false, true}));

  Dataset b;
  b.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}, {3, 4, 5}));
  b.setData("data_1", makeVariable<int>({Dim::X, 2}, {13, 14}));
  b.setLabels("edge_labels", makeVariable<int>({Dim::X, 3}, {23, 24, 25}));
  b.setLabels("labels", makeVariable<int>({Dim::X, 2}, {24, 25}));
  b.setMasks("masks", makeVariable<bool>({Dim::X, 2}, {false, true}));

  Dataset expected;
  expected.setCoord(Dim::X, makeVariable<int>({Dim::X, 5}, {1, 2, 3, 4, 5}));
  expected.setData("data_1", makeVariable<int>({Dim::X, 4}, {11, 12, 13, 14}));
  expected.setLabels("edge_labels",
                     makeVariable<int>({Dim::X, 5}, {21, 22, 23, 24, 25}));
  expected.setLabels("labels",
                     makeVariable<int>({Dim::X, 4}, {21, 22, 24, 25}));
  expected.setMasks(
      "masks", makeVariable<bool>({Dim::X, 4}, {false, true, false, true}));

  EXPECT_EQ(concatenate(a, b, Dim::X), expected);
}

TEST(ConcatenateTest, fail_when_histograms_have_non_overlapping_bins) {
  Dataset a;
  a.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}, {1, 2, 3}));
  a.setData("data_1", makeVariable<int>({Dim::X, 2}, {11, 12}));

  Dataset b;
  b.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}, {4, 5, 6}));
  b.setData("data_1", makeVariable<int>({Dim::X, 2}, {13, 14}));

  EXPECT_THROW(concatenate(a, b, Dim::X), except::MismatchError<Variable>);
}

TEST(ConcatenateTest, fail_mixing_point_data_and_histogram) {
  Dataset pointData;
  pointData.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}));
  pointData.setData("data_1", makeVariable<int>({Dim::X, 3}));

  Dataset histogram;
  histogram.setCoord(Dim::X, makeVariable<int>({Dim::X, 3}));
  histogram.setData("data_1", makeVariable<int>({Dim::X, 2}));

  EXPECT_THROW(concatenate(pointData, histogram, Dim::X), except::BinEdgeError);
}

TEST(ConcatenateTest, identical_non_dependant_data_is_copied) {
  const auto axis = makeVariable<int>({Dim::X, 3}, {1, 2, 3});
  const auto data = makeVariable<int>({Dim::X, 3}, {11, 12, 13});

  Dataset a;
  a.setCoord(Dim::X, axis);
  a.setData("data_1", data);

  Dataset b;
  b.setCoord(Dim::X, axis);
  b.setData("data_1", data);

  const auto d = concatenate(a, b, Dim::Y);

  EXPECT_EQ(d.coords()[Dim::X], axis);
  EXPECT_EQ(d["data_1"].data(), data);
}

TEST(ConcatenateTest, non_dependant_data_is_stacked) {
  const auto axis = makeVariable<int>({Dim::X, 3}, {1, 2, 3});

  Dataset a;
  a.setCoord(Dim::X, axis);
  a.setData("data_1", makeVariable<int>({Dim::X, 3}, {11, 12, 13}));

  Dataset b;
  b.setCoord(Dim::X, axis);
  b.setData("data_1", makeVariable<int>({Dim::X, 3}, {14, 15, 16}));

  const auto d = concatenate(a, b, Dim::Y);

  EXPECT_EQ(d["data_1"].data(), makeVariable<int>({{Dim::Y, 2}, {Dim::X, 3}},
                                                  {11, 12, 13, 14, 15, 16}));
}

TEST(ConcatenateTest, concat_2d_coord) {
  Dataset a;
  a.setCoord(Dim::X, makeVariable<double>({Dim::X, 3}, {1, 2, 3}));
  a.setData("data_1", makeVariable<double>({Dim::X, 3}, {11, 12, 13}));
  a.setLabels("label_1", makeVariable<int>({Dim::X, 3}, {21, 22, 23}));
  a.setMasks("mask_1", makeVariable<bool>({Dim::X, 3}, {false, true, false}));

  Dataset b(a);
  b.coords()[Dim::X] += 3;
  b["data_1"].data() += 100;

  Dataset expected;
  expected.setCoord(Dim::X,
                    makeVariable<double>({{Dim::Y, 4}, {Dim::X, 3}},
                                         {1, 2, 3, 4, 5, 6, 4, 5, 6, 1, 2, 3}));
  expected.setData("data_1", makeVariable<double>({{Dim::Y, 4}, {Dim::X, 3}},
                                                  {11, 12, 13, 111, 112, 113, 111,
                                                   112, 113, 11, 12, 13}));
  expected.setLabels("label_1", makeVariable<int>({Dim::X, 3}, {21, 22, 23}));
  expected.setMasks("mask_1",
                    makeVariable<bool>({Dim::X, 3}, {false, true, false}));

  const auto ab = concatenate(a, b, Dim::Y);
  const auto ba = concatenate(b, a, Dim::Y);
  const auto abba = concatenate(ab, ba, Dim::Y);

  EXPECT_EQ(abba, expected);
}
