// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/shape.h"

#include "test_data_arrays.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

template <class T> auto concat2(const T &a, const T &b, const Dim dim) {
  return concat(std::vector{a, b}, dim);
}

class Concatenate1DTest : public ::testing::Test {
protected:
  Concatenate1DTest()
      : a({{"data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13})}},
          {{Dim::X,
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})}}),
        b({{"data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{14, 15, 16})}},
          {{Dim::X,
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{4, 5, 6})}}) {
    a["data_1"].masks().set(
        "mask_1",
        makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false}));
    b["data_1"].masks().set(
        "mask_1",
        makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false}));
  }

  Dataset a;
  Dataset b;
};

TEST_F(Concatenate1DTest, simple_1d) {
  const auto d = concat2(a, b, Dim::X);

  EXPECT_EQ(d.coords()[Dim::X], makeVariable<int>(Dims{Dim::X}, Shape{6},
                                                  Values{1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(d["data_1"].data(),
            makeVariable<int>(Dims{Dim::X}, Shape{6},
                              Values{11, 12, 13, 14, 15, 16}));
  EXPECT_EQ(d["data_1"].masks()["mask_1"],
            makeVariable<bool>(Dims{Dim::X}, Shape{6},
                               Values{false, true, false, false, true, false}));
}

TEST_F(Concatenate1DTest, slices_of_1d) {
  EXPECT_EQ(concat2(a.slice({Dim::X, 0}), a.slice({Dim::X, 1}), Dim::X),
            a.slice({Dim::X, 0, 2}));
  EXPECT_EQ(concat2(a.slice({Dim::X, 0, 2}), a.slice({Dim::X, 2}), Dim::X), a);
  EXPECT_EQ(concat2(a.slice({Dim::X, 0}), a.slice({Dim::X, 1, 3}), Dim::X), a);
}

TEST_F(Concatenate1DTest, to_2d_with_0d_coord) {
  a.setCoord(Dim("label_0d"), makeVariable<int>(Values{1}));
  b.setCoord(Dim("label_0d"), makeVariable<int>(Values{2}));
  const auto ab = concat2(a, b, Dim::Y);
  EXPECT_EQ(ab["data_1"].data(),
            concat2(a["data_1"].data(), b["data_1"].data(), Dim::Y));
  const auto aba = concat2(ab, a, Dim::Y);
  EXPECT_EQ(aba["data_1"].data(),
            concat2(concat2(a["data_1"].data(), b["data_1"].data(), Dim::Y),
                    a["data_1"].data(), Dim::Y));
  const auto aab = concat2(a, ab, Dim::Y);
  EXPECT_EQ(aab["data_1"].data(),
            concat2(a["data_1"].data(),
                    concat2(a["data_1"].data(), b["data_1"].data(), Dim::Y),
                    Dim::Y));
}

TEST_F(Concatenate1DTest, empty_dataset) {
  a.erase("data_1");

  const auto ab = concat2(a, b, Dim::X);
  EXPECT_TRUE(ab.is_valid());
  EXPECT_EQ(ab, Dataset({}, {{Dim::X, concat2(a.coords()[Dim::X],
                                              b.coords()[Dim::X], Dim::X)}}));

  const auto ba = concat2(b, a, Dim::X);
  EXPECT_TRUE(ba.is_valid());
  EXPECT_EQ(ba, Dataset({}, {{Dim::X, concat2(b.coords()[Dim::X],
                                              a.coords()[Dim::X], Dim::X)}}));
}

TEST_F(Concatenate1DTest, non_overlapping_names) {
  a.setData("new_data", a.extract("data_1"));

  const auto ab = concat2(a, b, Dim::X);
  EXPECT_TRUE(ab.is_valid());
  EXPECT_EQ(ab, Dataset({}, {{Dim::X, concat2(a.coords()[Dim::X],
                                              b.coords()[Dim::X], Dim::X)}}));

  const auto ba = concat2(b, a, Dim::X);
  EXPECT_TRUE(ba.is_valid());
  EXPECT_EQ(ba, Dataset({}, {{Dim::X, concat2(b.coords()[Dim::X],
                                              a.coords()[Dim::X], Dim::X)}}));
}

TEST_F(Concatenate1DTest, sharing) {
  auto da1 = copy(a["data_1"]);
  auto da2 = copy(b["data_1"]);
  da2.coords().set(Dim::X, da1.coords()[Dim::X]);
  const auto out = concat2(da1, da2, Dim::Y);
  // Coords may be shared
  EXPECT_EQ(out.coords()[Dim::X], da1.coords()[Dim::X]);
  EXPECT_TRUE(out.coords()[Dim::X].is_same(da1.coords()[Dim::X]));
  // Masks are copied, just like in binary operations
  EXPECT_EQ(out.masks()["mask_1"], da1.masks()["mask_1"]);
  EXPECT_FALSE(out.masks()["mask_1"].is_same(da1.masks()["mask_1"]));
}

TEST_F(Concatenate1DTest, alignment_flag) {
  const auto d1 = concat2(a, b, Dim::X);
  EXPECT_TRUE(d1.coords()[Dim::X].is_aligned());

  a.coords().set_aligned(Dim::X, false);
  const auto d2 = concat2(a, b, Dim::X);
  EXPECT_TRUE(d2.coords()[Dim::X].is_aligned());

  b.coords().set_aligned(Dim::X, false);
  const auto d3 = concat2(a, b, Dim::X);
  EXPECT_TRUE(d3.coords()[Dim::X].is_aligned());

  a.coords().set_aligned(Dim::X, true);
  const auto d4 = concat2(a, b, Dim::X);
  EXPECT_TRUE(d4.coords()[Dim::X].is_aligned());

  a.setCoord(Dim("label_0d"), makeVariable<int>(Values{1}));
  b.setCoord(Dim("label_0d"), makeVariable<int>(Values{2}));
  a.coords().set_aligned(Dim("label_0d"), false);
  const auto d5 = concat2(a, b, Dim::X);
  EXPECT_TRUE(d5.coords()[Dim("label_0d")].is_aligned());

  b.coords().set_aligned(Dim("label_0d"), false);
  const auto d6 = concat2(a, b, Dim::X);
  EXPECT_FALSE(d6.coords()[Dim("label_0d")].is_aligned());
}

class Concatenate1DHistogramTest : public ::testing::Test {
protected:
  Concatenate1DHistogramTest()
      : a({{"data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{11, 12})}},
          {{Dim::X,
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})}}),
        b({{"data_1",
            makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{13, 14})}},
          {{Dim::X,
            makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{3, 4, 5})}}) {
    a["data_1"].masks().set("masks", makeVariable<bool>(Dims{Dim::X}, Shape{2},
                                                        Values{false, true}));
    b["data_1"].masks().set("masks", makeVariable<bool>(Dims{Dim::X}, Shape{2},
                                                        Values{false, true}));
  }

  Dataset a;
  Dataset b;
};

TEST_F(Concatenate1DHistogramTest, simple_1d) {
  Dataset expected({{"data_1", makeVariable<int>(Dims{Dim::X}, Shape{4},
                                                 Values{11, 12, 13, 14})}},
                   {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{5},
                                               Values{1, 2, 3, 4, 5})}});
  expected["data_1"].masks().set(
      "masks", makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                  Values{false, true, false, true}));

  EXPECT_EQ(concat2(a, b, Dim::X), expected);
}

TEST_F(Concatenate1DHistogramTest, slices_of_1d) {
  GTEST_SKIP() << "See #3148";

  EXPECT_EQ(concat2(a.slice({Dim::X, 0}), a.slice({Dim::X, 1}), Dim::X),
            a.slice({Dim::X, 0, 2}));
  EXPECT_EQ(concat2(a.slice({Dim::X, 0}), a.slice({Dim::X, 1, 2}), Dim::X), a);
  EXPECT_EQ(concat2(a.slice({Dim::X, 0, 1}), a.slice({Dim::X, 1}), Dim::X), a);
}

TEST_F(Concatenate1DHistogramTest, empty_dataset) {
  a.erase("data_1");

  const auto res = concat2(a, b, Dim::X);
  const auto expected_x =
      makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{1, 2, 3, 4, 5});
  EXPECT_TRUE(res.is_valid());
  EXPECT_EQ(res, Dataset({}, {{Dim::X, expected_x}}));
}

TEST(ConcatenateTest, fail_when_histograms_have_non_overlapping_bins) {
  const Dataset a(
      {{"data_1", makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{11, 12})}},
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})}});

  const Dataset b(
      {{"data_1", makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{13, 14})}},
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{4, 5, 6})}});

  EXPECT_THROW_DISCARD(concat2(a, b, Dim::X), except::VariableError);
}

TEST(ConcatenateTest, fail_mixing_point_data_and_histogram) {
  const Dataset pointData(
      {{"data_1", makeVariable<int>(Dims{Dim::X}, Shape{3})}},
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3})}});

  const Dataset histogram(
      {{"data_1", makeVariable<int>(Dims{Dim::X}, Shape{2})}},
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3})}});

  EXPECT_THROW_DISCARD(concat2(pointData, histogram, Dim::X),
                       except::BinEdgeError);
}

TEST(ConcatenateTest, identical_non_dependant_data_is_stacked) {
  const auto axis = makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto data =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13});

  const Dataset a({{"data_1", data}}, {{Dim::X, axis}});
  const Dataset b({{"data_1", data}}, {{Dim::X, axis}});

  const auto d = concat2(a, b, Dim::Y);

  EXPECT_EQ(d.coords()[Dim::X], axis);
  EXPECT_EQ(d["data_1"].data(),
            makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                              Values{11, 12, 13, 11, 12, 13}));
}

TEST(ConcatenateTest, non_dependant_data_is_stacked) {
  const auto axis = makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});

  const Dataset a({{"data_1", makeVariable<int>(Dims{Dim::X}, Shape{3},
                                                Values{11, 12, 13})}},
                  {{Dim::X, axis}});
  const Dataset b({{"data_1", makeVariable<int>(Dims{Dim::X}, Shape{3},
                                                Values{14, 15, 16})}},
                  {{Dim::X, axis}});

  const auto d = concat2(a, b, Dim::Y);

  EXPECT_EQ(d["data_1"].data(),
            makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                              Values{11, 12, 13, 14, 15, 16}));
}

TEST(ConcatenateTest, concat_2d_coord) {
  Dataset a(
      {{"data_1",
        makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{11, 12, 13})}},
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})},
       {Dim("label_1"),
        makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23})}});
  a["data_1"].masks().set(
      "mask_1",
      makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false}));

  Dataset b = copy(a);
  EXPECT_EQ(a, b);
  b.coords()[Dim::X] += 3 * sc_units::one;
  b["data_1"].data() += 100 * sc_units::one;

  Dataset expected(
      {{"data_1", makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{4, 3},
                                    Values{11, 12, 13, 111, 112, 113, 111, 112,
                                           113, 11, 12, 13})}},
      {{Dim::X, makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{4, 3},
                                  Values{1, 2, 3, 4, 5, 6, 4, 5, 6, 1, 2, 3})},
       {Dim("label_1"),
        makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23})}});
  expected["data_1"].masks().set(
      "mask_1",
      makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false}));

  const auto ab = concat2(a, b, Dim::Y);
  const auto ba = concat2(b, a, Dim::Y);
  const auto abba = concat2(ab, ba, Dim::Y);

  EXPECT_EQ(abba, expected);
}

TEST(ConcatenateTest, broadcast_coord) {
  DataArray a(1.0 * sc_units::one, {{Dim::X, 1.0 * sc_units::one}});
  DataArray b(makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 3}),
              {{Dim::X, 2.0 * sc_units::one}});
  EXPECT_EQ(
      concat2(a, b, Dim::X),
      DataArray(makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}),
                {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3},
                                               Values{1, 2, 2})}}));
  EXPECT_EQ(
      concat2(b, a, Dim::X),
      DataArray(makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2, 3, 1}),
                {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3},
                                               Values{2, 2, 1})}}));
}

class ConcatTest : public ::testing::Test {
protected:
  DataArray da = make_data_array_1d();
  DataArray da2 = concat(std::vector{da, da + da}, Dim::Y);
};

TEST_F(ConcatTest, empty) {
  EXPECT_THROW_DISCARD(concat(std::vector<DataArray>{}, Dim::X),
                       std::invalid_argument);
  EXPECT_THROW_DISCARD(concat(std::vector<Dataset>{}, Dim::X),
                       std::invalid_argument);
}

TEST_F(ConcatTest, single_existing_dim) {
  const auto out = concat(std::vector{da}, Dim::X);
  EXPECT_EQ(out, da);
  EXPECT_FALSE(out.data().is_same(da.data()));
}

TEST_F(ConcatTest, single_new_dim) {
  const auto out = concat(std::vector{da}, Dim::Y);
  EXPECT_EQ(out.slice({Dim::Y, 0}), da);
  EXPECT_FALSE(out.data().is_same(da.data()));
}

TEST_F(ConcatTest, multiple) {
  const auto expected =
      concat(std::vector{concat(std::vector{da2, da2}, Dim::Z), da2}, Dim::Z);
  EXPECT_EQ(concat(std::vector{da2, da2, da2}, Dim::Z), expected);
  auto a = da2;
  auto b = da2 + da2;
  auto c = da2 + da2 + da2;
  for (const auto &dim : {Dim::X, Dim::Y, Dim::Z}) {
    auto abc = concat(std::vector{a, b, c}, dim);
    auto ab_c = concat(std::vector{concat(std::vector{a, b}, dim), c}, dim);
    auto a_bc = concat(std::vector{a, concat(std::vector{b, c}, dim)}, dim);
    EXPECT_EQ(abc, ab_c);
    EXPECT_EQ(abc, a_bc);
  }
}

class ConcatHistogramTest : public ConcatTest {
protected:
  ConcatHistogramTest() {
    a = copy(da2);
    a.coords().set(
        Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
    b = copy(da2);
    b.coords().set(
        Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{3, 4, 5}));
    c = copy(da2);
    c.coords().set(
        Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{5, 6, 7}));
  }
  DataArray a;
  DataArray b;
  DataArray c;
};

TEST_F(ConcatHistogramTest, multiple_matching_edges) {
  for (const auto &dim : {Dim::X, Dim::Y, Dim::Z}) {
    auto abc = concat(std::vector{a, b, c}, dim);
    auto ab_c = concat(std::vector{concat(std::vector{a, b}, dim), c}, dim);
    auto a_bc = concat(std::vector{a, concat(std::vector{b, c}, dim)}, dim);
    EXPECT_EQ(abc, ab_c);
    EXPECT_EQ(abc, a_bc);
  }
}

TEST_F(ConcatHistogramTest, multiple_mismatching_edges) {
  EXPECT_THROW_DISCARD(concat(std::vector{a, c, b}, Dim::X),
                       except::VariableError);
  EXPECT_THROW_DISCARD(concat(std::vector{b, a, c}, Dim::X),
                       except::VariableError);
}

namespace {
const auto no_edges = [](auto da) {
  auto x = da.coords()[Dim::X];
  da.coords().set(
      Dim::X, concat(std::vector{x.slice({Dim::X, 0, 1}),
                                 x.slice({Dim::X, 2, da.dims()[Dim::X] + 1})},
                     Dim::X));
  return da;
};
} // namespace

TEST_F(ConcatHistogramTest, fail_mixing_point_data_and_histogram) {
  EXPECT_THROW_DISCARD(concat(std::vector{no_edges(a), b, c}, Dim::X),
                       except::BinEdgeError);
  EXPECT_THROW_DISCARD(concat(std::vector{a, no_edges(b), c}, Dim::X),
                       except::BinEdgeError);
  EXPECT_THROW_DISCARD(concat(std::vector{a, b, no_edges(c)}, Dim::X),
                       except::BinEdgeError);
  EXPECT_THROW_DISCARD(concat(std::vector{no_edges(a), no_edges(b), c}, Dim::X),
                       except::BinEdgeError);
  EXPECT_THROW_DISCARD(concat(std::vector{no_edges(a), b, no_edges(c)}, Dim::X),
                       except::BinEdgeError);
  EXPECT_THROW_DISCARD(concat(std::vector{a, no_edges(b), no_edges(c)}, Dim::X),
                       except::BinEdgeError);
  EXPECT_NO_THROW_DISCARD(
      concat(std::vector{no_edges(a), no_edges(b), no_edges(c)}, Dim::X));
}

TEST_F(ConcatHistogramTest, multiple_join_unrelated_dim) {
  // We have edges along Dim::X, this just gets concatenated, but since we have
  // an extra dim of length 2 it is also duplicated.
  auto out = concat(std::vector{a, c, b}, Dim::Y);
  EXPECT_EQ(out.coords()[Dim::X],
            concat(std::vector{a.coords()[Dim::X], a.coords()[Dim::X],
                               c.coords()[Dim::X], c.coords()[Dim::X],
                               b.coords()[Dim::X], b.coords()[Dim::X]},
                   Dim::Y));
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
  for (const auto &buffer2 :
       {buffer * (1.0 * sc_units::m),
        DataArray(data, {{Dim::X, data + data}},
                  {{"mask", 1.0 * sc_units::one}}, {}),
        DataArray(data, {{Dim::Y, data + data}, {Dim::X, data + data}}),
        DataArray(data, {})}) {
    auto var2 = make_bins(indices, Dim::Event, buffer2);
    EXPECT_THROW_DISCARD(concat2(var, var2, Dim::X), std::runtime_error);
    EXPECT_THROW_DISCARD(concat2(var, var2, Dim::Y), std::runtime_error);
    EXPECT_THROW_DISCARD(concat2(var2, var, Dim::X), std::runtime_error);
    EXPECT_THROW_DISCARD(concat2(var2, var, Dim::Y), std::runtime_error);
  }
}

TEST_F(ConcatenateBinnedTest, existing_dim) {
  auto out = concat2(var, var, Dim::X);
  EXPECT_EQ(out.slice({Dim::X, 0, 2}), var);
  EXPECT_EQ(out.slice({Dim::X, 2, 4}), var);
  out = concat2(var + 1.2 * sc_units::one, out, Dim::X);
  EXPECT_EQ(out.slice({Dim::X, 0, 2}), var + 1.2 * sc_units::one);
  EXPECT_EQ(out.slice({Dim::X, 2, 4}), var);
  EXPECT_EQ(out.slice({Dim::X, 4, 6}), var);
}

TEST_F(ConcatenateBinnedTest, new_dim) {
  auto out = concat2(var, var, Dim::Y);
  EXPECT_EQ(out.slice({Dim::Y, 0}), var);
  EXPECT_EQ(out.slice({Dim::Y, 1}), var);
  out = concat2(var + 1.2 * sc_units::one, out, Dim::Y);
  EXPECT_EQ(out.slice({Dim::Y, 0}), var + 1.2 * sc_units::one);
  EXPECT_EQ(out.slice({Dim::Y, 1}), var);
  EXPECT_EQ(out.slice({Dim::Y, 2}), var);
}

TEST_F(ConcatenateBinnedTest, empty_bins) {
  const Variable empty_indices =
      makeVariable<scipp::index_pair>(Dims{Dim::X}, Shape{0});
  const Variable empty = make_bins(empty_indices, Dim::Event, buffer);

  EXPECT_EQ(concat2(empty, empty, Dim::X), empty);
  EXPECT_EQ(concat2(empty, var, Dim::X), var);
  EXPECT_EQ(concat2(var, empty, Dim::X), var);
}
