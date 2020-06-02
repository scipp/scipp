// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/shape.h"

using namespace scipp;
using namespace scipp::dataset;

class Concatenate1DTest : public ::testing::Test {
protected:
  Concatenate1DTest() {
    a.setCoord(Dim::X,
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
    a.setData("data_1",
              makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13}));
    a.setCoord(Dim("label_1"),
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23}));
    a.setMask("mask_1", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                           Values{false, true, false}));

    b.setCoord(Dim::X,
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{4, 5, 6}));
    b.setData("data_1",
              makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{14, 15, 16}));
    b.setCoord(Dim("label_1"),
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{24, 25, 26}));
    b.setMask("mask_1", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                           Values{false, true, false}));
  }

  Dataset a;
  Dataset b;
};

TEST_F(Concatenate1DTest, simple_1d) {
  const auto d = concatenate(a, b, Dim::X);

  EXPECT_EQ(d.coords()[Dim::X], makeVariable<int>(Dims{Dim::X}, Shape{6},
                                                  Values{1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(d["data_1"].data(),
            makeVariable<int>(Dims{Dim::X}, Shape{6},
                              Values{11, 12, 13, 14, 15, 16}));
  EXPECT_EQ(d.coords()[Dim("label_1")],
            makeVariable<int>(Dims{Dim::X}, Shape{6},
                              Values{21, 22, 23, 24, 25, 26}));
  EXPECT_EQ(d.masks()["mask_1"],
            makeVariable<bool>(Dims{Dim::X}, Shape{6},
                               Values{false, true, false, false, true, false}));
}

TEST_F(Concatenate1DTest, to_2d_with_0d_coord) {
  a.setCoord(Dim("label_0d"), makeVariable<int>(Values{1}));
  b.setCoord(Dim("label_0d"), makeVariable<int>(Values{2}));
  const auto ab = concatenate(a, b, Dim::Y);
  EXPECT_EQ(ab["data_1"].data(),
            concatenate(a["data_1"].data(), b["data_1"].data(), Dim::Y));
  const auto aba = concatenate(ab, a, Dim::Y);
  EXPECT_EQ(
      aba["data_1"].data(),
      concatenate(concatenate(a["data_1"].data(), b["data_1"].data(), Dim::Y),
                  a["data_1"].data(), Dim::Y));
  const auto aab = concatenate(a, ab, Dim::Y);
  EXPECT_EQ(
      aab["data_1"].data(),
      concatenate(a["data_1"].data(),
                  concatenate(a["data_1"].data(), b["data_1"].data(), Dim::Y),
                  Dim::Y));
}

TEST(ConcatenateTest, simple_1d_histogram) {
  Dataset a;
  a.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  a.setData("data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{11, 12}));
  a.setCoord(Dim("edge_labels"),
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23}));
  a.setCoord(Dim("labels"),
             makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{21, 22}));
  a.setMask("masks",
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));

  Dataset b;
  b.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{3, 4, 5}));
  b.setData("data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{13, 14}));
  b.setCoord(Dim("edge_labels"),
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{23, 24, 25}));
  b.setCoord(Dim("labels"),
             makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{24, 25}));
  b.setMask("masks",
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));

  Dataset expected;
  expected.setCoord(
      Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{1, 2, 3, 4, 5}));
  expected.setData("data_1", makeVariable<int>(Dims{Dim::X}, Shape{4},
                                               Values{11, 12, 13, 14}));
  expected.setCoord(
      Dim("edge_labels"),
      makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{21, 22, 23, 24, 25}));
  expected.setCoord(Dim("labels"), makeVariable<int>(Dims{Dim::X}, Shape{4},
                                                     Values{21, 22, 24, 25}));
  expected.setMask("masks",
                   makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                      Values{false, true, false, true}));

  EXPECT_EQ(concatenate(a, b, Dim::X), expected);
}

TEST(ConcatenateTest, fail_when_histograms_have_non_overlapping_bins) {
  Dataset a;
  a.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  a.setData("data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{11, 12}));

  Dataset b;
  b.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{4, 5, 6}));
  b.setData("data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{13, 14}));

  EXPECT_THROW(concatenate(a, b, Dim::X), except::VariableMismatchError);
}

TEST(ConcatenateTest, fail_mixing_point_data_and_histogram) {
  Dataset pointData;
  pointData.setCoord(Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}));
  pointData.setData("data_1", makeVariable<int>(Dims{Dim::X}, Shape{3}));

  Dataset histogram;
  histogram.setCoord(Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}));
  histogram.setData("data_1", makeVariable<int>(Dims{Dim::X}, Shape{2}));

  EXPECT_THROW(concatenate(pointData, histogram, Dim::X), except::BinEdgeError);
}

TEST(ConcatenateTest, identical_non_dependant_data_is_copied) {
  const auto axis = makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto data =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13});

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
  const auto axis = makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});

  Dataset a;
  a.setCoord(Dim::X, axis);
  a.setData("data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13}));

  Dataset b;
  b.setCoord(Dim::X, axis);
  b.setData("data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{14, 15, 16}));

  const auto d = concatenate(a, b, Dim::Y);

  EXPECT_EQ(d["data_1"].data(),
            makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                              Values{11, 12, 13, 14, 15, 16}));
}

TEST(ConcatenateTest, concat_2d_coord) {
  Dataset a;
  a.setCoord(Dim::X,
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  a.setData("data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13}));
  a.setCoord(Dim("label_1"),
             makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23}));
  a.setMask("mask_1", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                         Values{false, true, false}));

  Dataset b(a);
  b.coords()[Dim::X] += 3 * units::one;
  b["data_1"].data() += 100 * units::one;

  Dataset expected;
  expected.setCoord(
      Dim::X, makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{4, 3},
                                Values{1, 2, 3, 4, 5, 6, 4, 5, 6, 1, 2, 3}));
  expected.setData("data_1",
                   makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{4, 3},
                                     Values{11, 12, 13, 111, 112, 113, 111, 112,
                                            113, 11, 12, 13}));
  expected.setCoord(Dim("label_1"), makeVariable<int>(Dims{Dim::X}, Shape{3},
                                                      Values{21, 22, 23}));
  expected.setMask("mask_1", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                                Values{false, true, false}));

  const auto ab = concatenate(a, b, Dim::Y);
  const auto ba = concatenate(b, a, Dim::Y);
  const auto abba = concatenate(ab, ba, Dim::Y);

  EXPECT_EQ(abba, expected);
}

TEST(ConcatenateTest, dataset_with_no_data_items) {
  Dataset a, b;
  a.setCoord(Dim::X,
             makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2}));
  a.setCoord(Dim("points"),
             makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{.1, .2}));
  b.setCoord(Dim::X,
             makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{3, 4}));
  b.setCoord(Dim("points"),
             makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{.3, .4}));

  const auto res = concatenate(a, b, Dim::X);

  EXPECT_EQ(res.coords()[Dim::X],
            makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4}));
  EXPECT_EQ(
      res.coords()[Dim("points")],
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{.1, .2, .3, .4}));
}

TEST(ConcatenateTest, dataset_with_no_data_items_histogram) {
  Dataset a, b;
  a.setCoord(Dim::X,
             makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  a.setCoord(Dim("histogram"),
             makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{.1, .2}));
  b.setCoord(Dim::X,
             makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{3, 4, 5}));
  b.setCoord(Dim("histogram"),
             makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{.3, .4}));

  const auto res = concatenate(a, b, Dim::X);

  EXPECT_EQ(res.coords()[Dim::X], makeVariable<double>(Dims{Dim::X}, Shape{5},
                                                       Values{1, 2, 3, 4, 5}));
  EXPECT_EQ(
      res.coords()[Dim("histogram")],
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{.1, .2, .3, .4}));
}
