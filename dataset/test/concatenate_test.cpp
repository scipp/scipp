// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
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
    a.setCoord("data_1", Dim("label_1"),
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23}));
    a["data_1"].masks().set(
        "mask_1",
        makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false}));

    b.setCoord(Dim::X,
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{4, 5, 6}));
    b.setData("data_1",
              makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{14, 15, 16}));
    b.setCoord("data_1", Dim("label_1"),
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{24, 25, 26}));
    b["data_1"].masks().set(
        "mask_1",
        makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false}));
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
  EXPECT_EQ(d["data_1"].attrs()[Dim("label_1")],
            makeVariable<int>(Dims{Dim::X}, Shape{6},
                              Values{21, 22, 23, 24, 25, 26}));
  EXPECT_EQ(d["data_1"].masks()["mask_1"],
            makeVariable<bool>(Dims{Dim::X}, Shape{6},
                               Values{false, true, false, false, true, false}));
}

TEST_F(Concatenate1DTest, slices_of_1d) {
  EXPECT_EQ(concatenate(a.slice({Dim::X, 0}), a.slice({Dim::X, 1}), Dim::X),
            a.slice({Dim::X, 0, 2}));
  EXPECT_EQ(concatenate(a.slice({Dim::X, 0, 2}), a.slice({Dim::X, 2}), Dim::X),
            a);
  EXPECT_EQ(concatenate(a.slice({Dim::X, 0}), a.slice({Dim::X, 1, 3}), Dim::X),
            a);
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

class Concatenate1DHistogramTest : public ::testing::Test {
protected:
  Concatenate1DHistogramTest() {
    a.setCoord(Dim::X,
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
    a.setData("data_1",
              makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{11, 12}));
    a.setCoord("data_1", Dim("edge_labels"),
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23}));
    a.setCoord("data_1", Dim("labels"),
               makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{21, 22}));
    a["data_1"].masks().set("masks", makeVariable<bool>(Dims{Dim::X}, Shape{2},
                                                        Values{false, true}));
    b.setCoord(Dim::X,
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{3, 4, 5}));
    b.setData("data_1",
              makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{13, 14}));
    b.setCoord("data_1", Dim("edge_labels"),
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{23, 24, 25}));
    b.setCoord("data_1", Dim("labels"),
               makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{24, 25}));
    b["data_1"].masks().set("masks", makeVariable<bool>(Dims{Dim::X}, Shape{2},
                                                        Values{false, true}));
  }

  Dataset a;
  Dataset b;
};

TEST_F(Concatenate1DHistogramTest, simple_1d) {
  Dataset expected;
  expected.setCoord(
      Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{1, 2, 3, 4, 5}));
  expected.setData("data_1", makeVariable<int>(Dims{Dim::X}, Shape{4},
                                               Values{11, 12, 13, 14}));
  expected.setCoord(
      "data_1", Dim("edge_labels"),
      makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{21, 22, 23, 24, 25}));
  expected.setCoord(
      "data_1", Dim("labels"),
      makeVariable<int>(Dims{Dim::X}, Shape{4}, Values{21, 22, 24, 25}));
  expected["data_1"].masks().set(
      "masks", makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                  Values{false, true, false, true}));

  EXPECT_EQ(concatenate(a, b, Dim::X), expected);
}

TEST_F(Concatenate1DHistogramTest, slices_of_1d) {
  EXPECT_EQ(concatenate(a.slice({Dim::X, 0}), a.slice({Dim::X, 1}), Dim::X),
            a.slice({Dim::X, 0, 2}));
  EXPECT_EQ(concatenate(a.slice({Dim::X, 0}), a.slice({Dim::X, 1, 2}), Dim::X),
            a);
  EXPECT_EQ(concatenate(a.slice({Dim::X, 0, 1}), a.slice({Dim::X, 1}), Dim::X),
            a);
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
  a["data_1"].masks().set(
      "mask_1",
      makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false}));

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
  expected["data_1"].masks().set(
      "mask_1",
      makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false}));

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

TEST(ConcatenateTest, broadcast_coord) {
  DataArray a(1.0 * units::one, {{Dim::X, 1.0 * units::one}});
  DataArray b(makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 3}),
              {{Dim::X, 2.0 * units::one}});
  EXPECT_EQ(
      concatenate(a, b, Dim::X),
      DataArray(makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}),
                {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3},
                                               Values{1, 2, 2})}}));
  EXPECT_EQ(
      concatenate(b, a, Dim::X),
      DataArray(makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2, 3, 1}),
                {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3},
                                               Values{2, 2, 1})}}));
}

class ConcatenateBinnedTest : public ::testing::Test {
protected:
  Variable indices = makeVariable<scipp::index_pair>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 5}});
  Variable data =
      makeVariable<double>(Dims{Dim::Event}, Shape{5}, Values{1, 2, 3, 4, 5});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable var = make_bins(indices, Dim::Event, buffer);
};

TEST_F(ConcatenateBinnedTest, mismatching_buffer) {
  for (const auto buffer2 :
       {buffer * (1.0 * units::m),
        DataArray(data, {{Dim::X, data + data}}, {{"mask", 1.0 * units::one}},
                  {}),
        DataArray(data, {{Dim::X, data + data}}, {},
                  {{Dim("attr"), 1.0 * units::one}}),
        DataArray(data, {{Dim::Y, data + data}, {Dim::X, data + data}}),
        DataArray(data, {})}) {
    auto var2 = make_bins(indices, Dim::Event, buffer2);
    EXPECT_THROW(concatenate(var, var2, Dim::X), std::runtime_error);
    EXPECT_THROW(concatenate(var, var2, Dim::Y), std::runtime_error);
    EXPECT_THROW(concatenate(var2, var, Dim::X), std::runtime_error);
    EXPECT_THROW(concatenate(var2, var, Dim::Y), std::runtime_error);
  }
}

TEST_F(ConcatenateBinnedTest, existing_dim) {
  auto out = concatenate(var, var, Dim::X);
  EXPECT_EQ(out.slice({Dim::X, 0, 2}), var);
  EXPECT_EQ(out.slice({Dim::X, 2, 4}), var);
  out = concatenate(var + 1.2 * units::one, out, Dim::X);
  EXPECT_EQ(out.slice({Dim::X, 0, 2}), var + 1.2 * units::one);
  EXPECT_EQ(out.slice({Dim::X, 2, 4}), var);
  EXPECT_EQ(out.slice({Dim::X, 4, 6}), var);
}

TEST_F(ConcatenateBinnedTest, new_dim) {
  auto out = concatenate(var, var, Dim::Y);
  EXPECT_EQ(out.slice({Dim::Y, 0}), var);
  EXPECT_EQ(out.slice({Dim::Y, 1}), var);
  out = concatenate(var + 1.2 * units::one, out, Dim::Y);
  EXPECT_EQ(out.slice({Dim::Y, 0}), var + 1.2 * units::one);
  EXPECT_EQ(out.slice({Dim::Y, 1}), var);
  EXPECT_EQ(out.slice({Dim::Y, 2}), var);
}
