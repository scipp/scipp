// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/reduction.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/shape.h"

#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

struct GroupbyTest : public ::testing::Test {
  GroupbyTest() {
    d.setData("a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                        units::m, Values{1, 2, 3, 1, 2, 3},
                                        Variances{4, 5, 6, 4, 5, 6}));
    d.setData("b", makeVariable<double>(Dimensions{Dim::X, 3}, units::s,
                                        Values{0.1, 0.2, 0.3}));
    d.setData("c", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                        units::s, Values{1, 2, 3, 4, 5, 6}));
    d["a"].attrs().set(Dim("scalar"), makeVariable<double>(Values{1.2}));
    d.setCoord(Dim("labels1"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                    units::m, Values{1, 2, 3}));
    d.setCoord(Dim("labels2"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                    units::m, Values{1, 1, 3}));
  }

  Dataset d;
};

TEST_F(GroupbyTest, fail_key_not_found) {
  EXPECT_THROW(groupby(d, Dim("invalid")), except::NotFoundError);
  EXPECT_THROW(groupby(d["a"], Dim("invalid")), except::NotFoundError);
}

TEST_F(GroupbyTest, fail_key_2d) {
  d.setCoord(Dim("2d"),
             makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                  units::s, Values{1, 2, 3, 4, 5, 6}));
  EXPECT_THROW(groupby(d, Dim("2d")), except::DimensionError);
  EXPECT_THROW(groupby(d["a"], Dim("2d")), except::DimensionError);
}

TEST_F(GroupbyTest, fail_key_with_variances) {
  d.setCoord(Dim("variances"),
             makeVariable<double>(Dimensions{Dim::X, 3}, units::m,
                                  Values{1, 2, 3}, Variances{4, 5, 6}));
  EXPECT_THROW(groupby(d, Dim("variances")), except::VariancesError);
  EXPECT_THROW(groupby(d["a"], Dim("variances")), except::VariancesError);
}

TEST_F(GroupbyTest, copy) {
  auto one_group = groupby(d, Dim("labels1"),
                           makeVariable<double>(Dims{Dim("labels1")}, Shape{2},
                                                units::m, Values{0, 4}));
  EXPECT_EQ(one_group.size(), 1);
  EXPECT_EQ(one_group.copy(0), d);

  auto two_groups = groupby(d, Dim("labels1"),
                            makeVariable<double>(Dims{Dim("labels1")}, Shape{3},
                                                 units::m, Values{0, 3, 4}));
  EXPECT_EQ(two_groups.size(), 2);
  EXPECT_EQ(two_groups.copy(0), d.slice({Dim::X, 0, 2}));
  EXPECT_EQ(two_groups.copy(1), d.slice({Dim::X, 2, 3}));
}

TEST_F(GroupbyTest, copy_throws_with_out_of_bounds_index) {
  auto two_groups = groupby(d, Dim("labels1"),
                            makeVariable<double>(Dims{Dim("labels1")}, Shape{3},
                                                 units::m, Values{0, 3, 4}));
  EXPECT_THROW_DISCARD(two_groups.copy(3), std::out_of_range);
}

struct GroupbyTestCopyMultipleSubgroupsTest : public ::testing::Test {
protected:
  const Variable var =
      makeVariable<double>(Dims{Dim::X}, Shape{12}, units::m,
                           Values{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
  const Variable labels = makeVariable<double>(
      Dims{Dim::X}, Shape{12}, Values{0, 1, 1, 0, 2, 2, 0, 0, 1, 0, 1, 2});

  const Variable var0 = makeVariable<double>(Dims{Dim::X}, Shape{5}, units::m,
                                             Values{0, 3, 6, 7, 9});
  const Variable var1 = makeVariable<double>(Dims{Dim::X}, Shape{4}, units::m,
                                             Values{1, 2, 8, 10});
  const Variable var2 =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, units::m, Values{4, 5, 11});
  const Variable labels0 =
      makeVariable<double>(Dims{Dim::X}, Shape{5}, Values{0, 0, 0, 0, 0});
  const Variable labels1 =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 1, 1, 1});
  const Variable labels2 =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2, 2, 2});
  const DataArray da0 = DataArray(var0, {{Dim("labels"), labels0}});
  const DataArray da1 = DataArray(var1, {{Dim("labels"), labels1}});
  const DataArray da2 = DataArray(var2, {{Dim("labels"), labels2}});
};

TEST_F(GroupbyTestCopyMultipleSubgroupsTest, no_edges) {
  const auto da = DataArray(var, {{Dim("labels"), labels}});

  auto grouped = groupby(da, Dim("labels"));

  EXPECT_EQ(grouped.copy(0), da0);
  EXPECT_EQ(grouped.copy(1), da1);
  EXPECT_EQ(grouped.copy(2), da2);
}

TEST_F(GroupbyTestCopyMultipleSubgroupsTest, with_edges) {
  const auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{13}, units::m,
                           Values{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  const auto da = DataArray(var, {{Dim::X, edges}, {Dim("labels"), labels}},
                            {{"mask", edges}}, {{Dim("attr"), edges}});

  auto grouped = groupby(da, Dim("labels"));

  for (scipp::index i : {0, 1, 2}) {
    const auto out = grouped.copy(i);
    EXPECT_FALSE(out.coords().contains(Dim::X));
    EXPECT_FALSE(out.masks().contains("mask"));
    EXPECT_FALSE(out.attrs().contains(Dim("attr")));
  }
  EXPECT_EQ(grouped.copy(0), da0);
  EXPECT_EQ(grouped.copy(1), da1);
  EXPECT_EQ(grouped.copy(2), da2);
}

TEST_F(GroupbyTest, copy_nan) {
  constexpr auto nan = std::numeric_limits<double>::quiet_NaN();
  DataArray da{
      makeVariable<double>(Dims{Dim::X}, Shape{6}, Values{0, 1, 2, 3, 4, 5})};
  da.coords().set(Dim("labels"),
                  makeVariable<double>(Dims{Dim::X}, Shape{6},
                                       Values{0.0, nan, 1.0, 0.0, 3.0, 3.0}));

  auto by_dim = groupby(da, Dim("labels"));
  EXPECT_EQ(by_dim.size(), 4);
  EXPECT_EQ(by_dim.copy(0).data(),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 3}));
  EXPECT_EQ(by_dim.copy(1).data(),
            makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{2}));
  EXPECT_EQ(by_dim.copy(2).data(),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{4, 5}));
  EXPECT_EQ(by_dim.copy(3).data(),
            makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1}));

  auto by_bins = groupby(
      da, Dim("labels"),
      makeVariable<double>(Dims{Dim("labels")}, Shape{3}, Values{0, 2, 5}));
  EXPECT_EQ(by_bins.size(), 2);
  EXPECT_EQ(by_bins.copy(0).data(),
            makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 2, 3}));
  EXPECT_EQ(by_bins.copy(1).data(),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{4, 5}));
}

TEST_F(GroupbyTest, drop_2d_coord) {
  d.setCoord(Dim("2d"), makeVariable<float>(Dims{Dim::X, Dim::Z}, Shape{3, 2}));
  EXPECT_NO_THROW(groupby(d, Dim("labels2")));
  EXPECT_FALSE(
      groupby(d, Dim("labels2")).sum(Dim::X).coords().contains(Dim("2d")));
}

TEST_F(GroupbyTest, dataset_1d_and_2d) {
  Dataset expected;
  Dim dim("labels2");
  expected.setData("a",
                   makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2},
                                        units::m, Values{1.5, 3.0, 1.5, 3.0},
                                        Variances{9.0 / 4, 6.0, 9.0 / 4, 6.0}));
  expected.setData("b", makeVariable<double>(Dims{dim}, Shape{2}, units::s,
                                             Values{(0.1 + 0.2) / 2.0, 0.3}));
  expected.setData("c",
                   makeVariable<double>(Dims{Dim(Dim::Z), dim}, Shape{2, 2},
                                        units::s, Values{1.5, 3.0, 4.5, 6.0}));
  expected["a"].attrs().set(Dim("scalar"), makeVariable<double>(Values{1.2}));
  expected.setCoord(
      dim, makeVariable<double>(Dims{dim}, Shape{2}, units::m, Values{1, 3}));

  EXPECT_EQ(groupby(d, dim).mean(Dim::X), expected);
  EXPECT_EQ(groupby(d["a"], dim).mean(Dim::X), expected["a"]);
  EXPECT_EQ(groupby(d["b"], dim).mean(Dim::X), expected["b"]);
  EXPECT_EQ(groupby(d["c"], dim).mean(Dim::X), expected["c"]);
}

TEST_F(GroupbyTest, array_variable) {
  auto const var =
      makeVariable<double>(Dimensions{Dim::X, 3}, Values{1.0, 1.1, 2.5});

  DataArray arr{
      makeVariable<int>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}},
                        Values{1, 2, 3, 4, 5, 6}),
      {{Dim::Y, makeVariable<int>(Dimensions{Dim::Y, 2}, Values{1, 2})},
       {Dim::X, makeVariable<int>(Dimensions{Dim::X, 3}, Values{1, 2, 3})},
       {Dim("labels2"), var}},
  };

  auto bins =
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, Values{0.0, 1.0, 2.0, 3.0});

  auto const groupby_label = groupby(arr, Dim("labels2"), bins);
  auto const groupby_variable = groupby(arr, var, bins);

  EXPECT_EQ(groupby_label.key(), groupby_variable.key());

  auto const var_bad =
      makeVariable<double>(Dimensions{Dim::X, 4}, Values{1.0, 1.1, 2.5, 9.0});

  EXPECT_THROW(groupby(arr, var_bad, bins), except::DimensionError);
}

TEST_F(GroupbyTest, by_attr) {
  auto da = copy(d["a"]);
  const auto key = Dim("labels1");
  const auto grouped_coord = groupby(da, key).sum(Dim::X);
  da.attrs().set(key, da.coords().extract(key));
  const auto grouped_attr = groupby(da, key).sum(Dim::X);
  EXPECT_EQ(grouped_coord, grouped_attr);
}

struct GroupbyMaskedTest : public GroupbyTest {
  GroupbyMaskedTest() : GroupbyTest() {
    for (const auto &item : {"a", "b", "c"})
      d[item].masks().set("mask_x",
                          makeVariable<bool>(Dimensions{Dim::X, 3},
                                             Values{false, true, false}));
    for (const auto &item : {"a", "c"})
      d[item].masks().set("mask_z", makeVariable<bool>(Dimensions{Dim::Z, 2},
                                                       Values{false, true}));
  }
};

TEST_F(GroupbyMaskedTest, sum) {
  Dataset expected;
  const Dim dim("labels2");
  expected.setData("a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2},
                                             units::m, Values{1, 3, 1, 3},
                                             Variances{4, 6, 4, 6}));
  expected.setData("b", makeVariable<double>(Dimensions{dim, 2}, units::s,
                                             Values{0.1, 0.3}));
  expected.setData("c", makeVariable<double>(Dimensions{{Dim::Z, 2}, {dim, 2}},
                                             units::s, Values{1, 3, 4, 6}));
  expected.setCoord(
      dim, makeVariable<double>(Dimensions{dim, 2}, units::m, Values{1, 3}));
  expected["a"].attrs().set(Dim("scalar"), makeVariable<double>(Values{1.2}));
  for (const auto &item : {"a", "c"})
    expected[item].masks().set(
        "mask_z",
        makeVariable<bool>(Dimensions{Dim::Z, 2}, Values{false, true}));

  const auto result = groupby(d, dim).sum(Dim::X);
  EXPECT_EQ(result, expected);
  // Ensure reduction operation does NOT share the unrelated mask
  result["a"].masks()["mask_z"] |= true * units::none;
  EXPECT_NE(result["a"].masks()["mask_z"], d["a"].masks()["mask_z"]);
}

TEST_F(GroupbyMaskedTest, sum_irrelevant_mask) {
  Dataset expected;
  const Dim dim("labels2");
  expected.setData("a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2},
                                             units::m, Values{3, 3, 3, 3},
                                             Variances{9, 6, 9, 6}));
  expected.setData("b", makeVariable<double>(Dimensions{dim, 2}, units::s,
                                             Values{0.1 + 0.2, 0.3}));
  expected.setData("c", makeVariable<double>(Dimensions{{Dim::Z, 2}, {dim, 2}},
                                             units::s, Values{3, 3, 9, 6}));
  expected.setCoord(
      dim, makeVariable<double>(Dimensions{dim, 2}, units::m, Values{1, 3}));
  expected["a"].attrs().set(Dim("scalar"), makeVariable<double>(Values{1.2}));
  for (const auto &item : {"a", "c"})
    expected[item].masks().set(
        "mask_z",
        makeVariable<bool>(Dimensions{Dim::Z, 2}, Values{false, true}));

  for (const auto &item : {"a", "b", "c"})
    d[item].masks().erase("mask_x");
  auto result = groupby(d, dim).sum(Dim::X);
  EXPECT_EQ(result, expected);

  for (const auto &item : {"a", "c"}) {
    d[item].masks().erase("mask_z");
    ASSERT_TRUE(d[item].masks().empty());
  }
  const auto expected2 = groupby(d, dim).sum(Dim::X);
  for (const auto &item : {"a", "c"})
    result[item].masks().erase("mask_z");
  EXPECT_EQ(result, expected2);
}

TEST_F(GroupbyMaskedTest, mean_mask_ignores_values_properly) {
  // the mask is on a coordinate that the label does not include
  // this test verifies that the data is not affected
  Dataset expected;
  const Dim dim("labels2");
  expected.setData("a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2},
                                             units::m, Values{1, 3, 1, 3},
                                             Variances{4, 6, 4, 6}));
  expected.setData("b", makeVariable<double>(Dimensions{dim, 2}, units::s,
                                             Values{0.1, 0.3}));
  expected.setData("c", makeVariable<double>(Dimensions{{Dim::Z, 2}, {dim, 2}},
                                             units::s, Values{1, 3, 4, 6}));
  expected.setCoord(
      dim, makeVariable<double>(Dimensions{dim, 2}, units::m, Values{1, 3}));
  expected["a"].attrs().set(Dim("scalar"), makeVariable<double>(Values{1.2}));
  for (const auto &item : {"a", "c"})
    expected[item].masks().set(
        "mask_z",
        makeVariable<bool>(Dimensions{Dim::Z, 2}, Values{false, true}));

  const auto result = groupby(d, dim).mean(Dim::X);
  EXPECT_EQ(result, expected);
}

TEST_F(GroupbyMaskedTest, mean) {
  const auto result = groupby(d, Dim("labels1")).mean(Dim::X);

  EXPECT_EQ(result["a"].template values<double>()[0], 1.0);
  EXPECT_TRUE(std::isnan(result["a"].template values<double>()[1]));
  EXPECT_EQ(result["a"].template values<double>()[2], 3.0);

  EXPECT_EQ(result["a"].template variances<double>()[0], 4.0);
  EXPECT_TRUE(std::isnan(result["a"].template variances<double>()[1]));
  EXPECT_EQ(result["a"].template variances<double>()[2], 6.0);

  EXPECT_EQ(result["b"].template values<double>()[0], 0.1);
  EXPECT_TRUE(std::isnan(result["b"].template values<double>()[1]));
  EXPECT_EQ(result["b"].template values<double>()[2], 0.3);

  EXPECT_EQ(result["c"].template values<double>()[0], 1.0);
  EXPECT_TRUE(std::isnan(result["c"].template values<double>()[1]));
  EXPECT_EQ(result["c"].template values<double>()[2], 3.0);
  EXPECT_EQ(result["c"].template values<double>()[3], 4.0);
  EXPECT_TRUE(std::isnan(result["c"].template values<double>()[4]));
  EXPECT_EQ(result["c"].template values<double>()[5], 6.0);
}

TEST_F(GroupbyMaskedTest, mean2) {
  for (const auto &item : {"a", "b", "c"})
    d[item].masks().set(
        "mask_x",
        makeVariable<bool>(Dimensions{Dim::X, 3}, Values{false, false, true}));

  const Dim dim("labels2");
  const auto result = groupby(d, dim).mean(Dim::X);

  EXPECT_EQ(result["a"].template values<double>()[0], 1.5);
  EXPECT_TRUE(std::isnan(result["a"].template values<double>()[1]));
  EXPECT_EQ(result["a"].template variances<double>()[0], 2.25);
  EXPECT_TRUE(std::isnan(result["a"].template variances<double>()[1]));

  EXPECT_DOUBLE_EQ(result["b"].template values<double>()[0], 0.15);
  EXPECT_TRUE(std::isnan(result["b"].template values<double>()[1]));

  EXPECT_EQ(result["c"].template values<double>()[0], 1.5);
  EXPECT_TRUE(std::isnan(result["c"].template values<double>()[1]));
  EXPECT_EQ(result["c"].template values<double>()[2], 4.5);
  EXPECT_TRUE(std::isnan(result["c"].template values<double>()[3]));

  EXPECT_EQ(
      result.coords()[dim],
      makeVariable<double>(Dimensions{dim, 2}, units::m, Values{1.0, 3.0}));
}

TEST(GroupbyMaskedDataArrayTest, sum) {
  DataArray arr{
      makeVariable<int>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}},
                        Values{1, 2, 3, 4, 5, 6}),
      {{Dim::Y, makeVariable<int>(Dimensions{Dim::Y, 2}, Values{1, 2})},
       {Dim::X, makeVariable<int>(Dimensions{Dim::X, 3}, Values{1, 2, 3})},
       {Dim("labels"),
        makeVariable<double>(Dimensions{Dim::X, 3}, Values{1, 1, 3})}},
      {{"masks", makeVariable<bool>(Dimensions{Dim::X, 3},
                                    Values{false, true, false})}}};

  const Dim dim("labels");
  DataArray expected{
      makeVariable<int>(Dimensions{{Dim::Y, 2}, {dim, 2}}, Values{1, 3, 4, 6}),
      {{Dim::Y, makeVariable<int>(Dimensions{Dim::Y, 2}, Values{1, 2})},
       {dim, makeVariable<double>(Dimensions{dim, 2}, Values{1, 3})}}};

  EXPECT_EQ(groupby(arr, dim).sum(Dim::X), expected);
}

TEST(GroupbyMaskedDataArrayTest, mean) {
  DataArray arr{
      makeVariable<int>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}},
                        Values{1, 2, 3, 4, 5, 6}),
      {{Dim::Y, makeVariable<int>(Dimensions{Dim::Y, 2}, Values{1, 2})},
       {Dim::X, makeVariable<int>(Dimensions{Dim::X, 3}, Values{1, 2, 3})},
       {Dim("labels"),
        makeVariable<double>(Dimensions{Dim::X, 3}, Values{1, 2, 3})}},
      {{"masks", makeVariable<bool>(Dimensions{Dim::X, 3},
                                    Values{false, true, false})}}};

  const auto result = groupby(arr, Dim("labels")).mean(Dim::X);

  EXPECT_EQ(result.template values<double>()[0], 1.0);
  EXPECT_TRUE(std::isnan(result.template values<double>()[1]));
  EXPECT_EQ(result.template values<double>()[2], 3.0);
  EXPECT_EQ(result.template values<double>()[3], 4.0);
  EXPECT_TRUE(std::isnan(result.template values<double>()[4]));
  EXPECT_EQ(result.template values<double>()[5], 6.0);
}

TEST(GroupbyMaskedDataArrayTest, mean2) {
  DataArray arr{
      makeVariable<int>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}},
                        Values{1, 2, 3, 4, 5, 6}),
      {{Dim::Y, makeVariable<int>(Dimensions{Dim::Y, 2}, Values{1, 2})},
       {Dim::X, makeVariable<int>(Dimensions{Dim::X, 3}, Values{1, 2, 3})},
       {Dim("labels"),
        makeVariable<double>(Dimensions{Dim::X, 3}, Values{1, 1, 3})}},
      {{"masks", makeVariable<bool>(Dimensions{Dim::X, 3},
                                    Values{false, false, true})}}};

  const auto result = groupby(arr, Dim("labels")).mean(Dim::X);

  EXPECT_EQ(result.template values<double>()[0], 1.5);
  EXPECT_TRUE(std::isnan(result.template values<double>()[1]));
  EXPECT_EQ(result.template values<double>()[2], 4.5);
  EXPECT_TRUE(std::isnan(result.template values<double>()[3]));
}

struct GroupbyWithBinsTest : public ::testing::Test {
  GroupbyWithBinsTest() {
    d.setData("a", makeVariable<double>(Dimensions{Dim::X, 5}, units::s,
                                        Values{0.1, 0.2, 0.3, 0.4, 0.5}));
    d.setData("b", makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 5}},
                                        units::s,
                                        Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
    d["a"].attrs().set(Dim("scalar"), makeVariable<double>(Values{1.2}));
    d.setCoord(Dim("labels1"),
               makeVariable<double>(Dimensions{Dim::X, 5}, units::m,
                                    Values{1, 2, 3, 4, 5}));
    d.setCoord(Dim("labels2"),
               makeVariable<double>(Dimensions{Dim::X, 5}, units::m,
                                    Values{1.0, 1.1, 2.5, 4.0, 1.2}));
  }

  Dataset d;
};

TEST_F(GroupbyWithBinsTest, bins) {
  auto bins = makeVariable<double>(Dims{Dim::Z}, Shape{4}, units::m,
                                   Values{0.0, 1.0, 2.0, 3.0});

  Dataset expected;
  expected.setData("a", makeVariable<double>(Dims{Dim::Z}, Shape{3}, units::s,
                                             Values{0.0, 0.8, 0.3}));
  expected.setData("b",
                   makeVariable<double>(Dims{Dim::Y, Dim::Z}, Shape{2, 3},
                                        units::s, Values{0, 8, 3, 0, 23, 8}));
  expected.setCoord(Dim::Z, bins);
  expected["a"].attrs().set(Dim("scalar"), makeVariable<double>(Values{1.2}));

  EXPECT_EQ(groupby(d, Dim("labels2"), bins).sum(Dim::X), expected);
  EXPECT_EQ(groupby(d["a"], Dim("labels2"), bins).sum(Dim::X), expected["a"]);
  EXPECT_EQ(groupby(d["b"], Dim("labels2"), bins).sum(Dim::X), expected["b"]);
}

TEST_F(GroupbyWithBinsTest, bins_mean_empty) {
  auto bins = makeVariable<double>(Dims{Dim::Z}, Shape{4}, units::m,
                                   Values{0.0, 1.0, 2.0, 3.0});

  const auto binned = groupby(d, Dim("labels2"), bins).mean(Dim::X);
  EXPECT_TRUE(std::isnan(binned["a"].values<double>()[0]));
  EXPECT_FALSE(std::isnan(binned["a"].values<double>()[1]));
  EXPECT_TRUE(std::isnan(binned["b"].values<double>()[0]));
  EXPECT_TRUE(std::isnan(binned["b"].values<double>()[3]));
  EXPECT_FALSE(std::isnan(binned["b"].values<double>()[1]));
}

TEST_F(GroupbyWithBinsTest, single_bin) {
  auto bins =
      makeVariable<double>(Dims{Dim::Z}, Shape{2}, units::m, Values{1.0, 5.0});
  const auto groups = groupby(d, Dim("labels2"), bins);

  // Non-range slice drops Dim::Z, so the result must be equal to a global `sum`
  // or `mean` with the corresponding coord (the edges) attr added.
  const auto add_bins = [&bins](auto data) {
    data["a"].attrs().set(Dim("z"), bins);
    data["b"].attrs().set(Dim("z"), bins);
    return data;
  };
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 0}), add_bins(sum(d, Dim::X)));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 0}), add_bins(mean(d, Dim::X)));
}

TEST_F(GroupbyWithBinsTest, two_bin) {
  auto bins = makeVariable<double>(Dims{Dim::Z}, Shape{3}, units::m,
                                   Values{1.0, 2.0, 5.0});
  const auto groups = groupby(d, Dim("labels2"), bins);

  const auto add_bins = [&bins](auto data, const scipp::index bin) {
    data["a"].attrs().set(Dim("z"), bins.slice({Dim::Z, bin, bin + 2}));
    data["b"].attrs().set(Dim("z"), bins.slice({Dim::Z, bin, bin + 2}));
    return data;
  };

  auto group0 = concat(
      std::vector{d.slice({Dim::X, 0, 2}), d.slice({Dim::X, 4, 5})}, Dim::X);
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 0}),
            add_bins(sum(group0, Dim::X), 0));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 0}),
            add_bins(mean(group0, Dim::X), 0));

  const auto group1 = d.slice({Dim::X, 2, 4});
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 1}),
            add_bins(sum(group1, Dim::X), 1));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 1}),
            add_bins(mean(group1, Dim::X), 1));
}

TEST_F(GroupbyWithBinsTest, dataset_variable) {
  auto bins =
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, units::Unit(units::m),
                           Values{0.0, 1.0, 2.0, 3.0});

  auto const var =
      makeVariable<double>(Dimensions{Dim::X, 5}, units::Unit(units::m),
                           Values{1.0, 1.1, 2.5, 4.0, 1.2});

  d.setCoord(Dim("labels2"), var);

  auto const groupby_label = groupby(d, Dim("labels2"), bins);
  auto const groupby_variable = groupby(d, var, bins);

  EXPECT_EQ(groupby_label.key(), groupby_variable.key());

  auto const var_bad = makeVariable<double>(Dimensions{Dim::X, 1},
                                            units::Unit(units::m), Values{1.0});

  EXPECT_THROW(groupby(d, var_bad, bins), except::DimensionError);
}

auto make_events_in() {
  const auto weights = makeVariable<double>(Dims{Dim::Event}, Shape{7},
                                            Values{1, 2, 1, 3, 1, 4, 1},
                                            Variances{1, 5, 1, 6, 1, 7, 1});
  const auto x = makeVariable<double>(Dims{Dim::Event}, Shape{7},
                                      Values{1, 2, 3, 4, 5, 6, 7});
  const auto indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::Y}, Shape{3},
      Values{std::pair{0, 3}, std::pair{3, 5}, std::pair{5, 7}});
  const DataArray table(weights, {{Dim::X, x}});
  return make_bins(indices, Dim::Event, table);
}

auto make_events_out(bool mask = false) {
  const auto weights = makeVariable<double>(Dims{Dim::Event}, Shape{7},
                                            Values{1, 2, 1, 3, 1, 4, 1},
                                            Variances{1, 5, 1, 6, 1, 7, 1});
  const auto x = makeVariable<double>(Dims{Dim::Event}, Shape{7},
                                      Values{1, 2, 3, 4, 5, 6, 7});
  const auto indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim("labels")}, Shape{2},
      Values{mask ? std::pair{0, 3} : std::pair{0, 5}, std::pair{5, 7}});
  const DataArray table(weights, {{Dim::X, x}});
  return make_bins(indices, Dim::Event, table);
}

struct GroupbyBinnedTest : public ::testing::Test {
  DataArray a{
      make_events_in(),
      {{Dim("0-d"), makeVariable<double>(Values{1.2})},
       {Dim("labels"), makeVariable<int64_t>(Dims{Dim::Y}, Shape{3}, units::m,
                                             Values{1, 1, 3})}},
      {},
      {{Dim("scalar_attr"), makeVariable<double>(Values{1.2})}}};

  DataArray expected{
      make_events_out(),
      {{Dim("0-d"), makeVariable<double>(Values{1.2})},
       {Dim("labels"), makeVariable<int64_t>(Dims{Dim("labels")}, Shape{2},
                                             units::m, Values{1, 3})}},
      {},
      {{Dim("scalar_attr"), makeVariable<double>(Values{1.2})}}};
};

TEST_F(GroupbyBinnedTest, min_data_array) {
  expected.setData(
      makeVariable<double>(expected.dims(), Values{1, 1}, Variances{1, 1}));
  EXPECT_EQ(groupby(a, Dim("labels")).min(Dim::Y), expected);
}

TEST_F(GroupbyBinnedTest, max_data_array) {
  expected.setData(
      makeVariable<double>(expected.dims(), Values{3, 4}, Variances{6, 7}));
  EXPECT_EQ(groupby(a, Dim("labels")).max(Dim::Y), expected);
}

TEST_F(GroupbyBinnedTest, sum_data_array) {
  expected.setData(
      makeVariable<double>(expected.dims(), Values{8, 5}, Variances{14, 8}));
  EXPECT_EQ(groupby(a, Dim("labels")).sum(Dim::Y), expected);
}

TEST_F(GroupbyBinnedTest, mean_data_array) {
  EXPECT_THROW_DISCARD(groupby(a, Dim("labels")).mean(Dim::Y),
                       except::NotImplementedError);
}

TEST_F(GroupbyBinnedTest, sum_with_event_mask) {
  auto bins = bins_view<DataArray>(a.data());
  bins.masks().set("mask", equal(bins.data(), bins.data()));
  // Event masks not supported yet in reduction ops.
  EXPECT_THROW_DISCARD(groupby(a, Dim("labels")).sum(Dim::Y),
                       except::NotImplementedError);
}

TEST_F(GroupbyBinnedTest, concatenate_data_array) {
  EXPECT_EQ(groupby(a, Dim("labels")).concat(Dim::Y), expected);
}

TEST_F(GroupbyBinnedTest, concatenate_by_attr) {
  const auto key = Dim("labels");
  const auto grouped_coord = groupby(a, key).concat(Dim::Y);
  a.attrs().set(key, a.coords().extract(key));
  const auto grouped_attr = groupby(a, key).concat(Dim::Y);
  EXPECT_EQ(grouped_coord, grouped_attr);
}

TEST_F(GroupbyBinnedTest, concatenate_data_array_2d) {
  a = bin(a, {makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 8})});
  auto grouped = groupby(a, Dim("labels")).concat(Dim::Y);
  grouped.coords().erase(Dim::X);
  EXPECT_EQ(grouped.slice({Dim::X, 0}), expected);
  // Dim added by grouping is *outer* dim
  EXPECT_EQ(grouped.dims(), Dimensions({Dim("labels"), Dim::X}, {2, 1}));
}

TEST_F(GroupbyBinnedTest, concatenate_data_array_conflicting_2d_coord) {
  a = bin(a, {makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 3, 8})});
  a.coords().set(
      Dim::X, makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3}, units::m,
                                   Values{1, 3, 8, 1, 3, 9, 1, 3, 10}));
  auto grouped = groupby(a, Dim("labels")).concat(Dim::Y);
  EXPECT_EQ(
      grouped.coords().extract(Dim::X),
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{1, 10}));
  EXPECT_EQ(grouped.slice({Dim::X, 0}), expected);
}

TEST_F(GroupbyBinnedTest, concatenate_dataset) {
  const Dataset d{{{"a", a}, {"b", a}}};
  const Dataset expected_d{{{"a", expected}, {"b", expected}}};
  EXPECT_EQ(groupby(d, Dim("labels")).concat(Dim::Y), expected_d);
}

struct GroupbyBinnedMaskTest : public ::testing::Test {
  const DataArray a{
      make_events_in(),
      {{Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{3})},
       {Dim("labels"), makeVariable<double>(Dims{Dim::Y}, Shape{3}, units::m,
                                            Values{1, 1, 3})}},
      {{"mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{3},
                                     Values{false, true, false})}}};
  const DataArray expected{
      make_events_out(true),
      {{Dim("labels"), makeVariable<double>(Dims{Dim("labels")}, Shape{2},
                                            units::m, Values{1, 3})}}};
};

TEST_F(GroupbyBinnedMaskTest, concatenate) {
  EXPECT_EQ(groupby(a, Dim("labels")).concat(Dim::Y), expected);
}

struct GroupbyLogicalTest : public ::testing::Test {
  GroupbyLogicalTest() {
    d.setData(
        "a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                Values{true, false, false, true, true, false}));
    d.setCoord(Dim("labels1"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                    units::m, Values{1, 2, 3}));
    d.setCoord(Dim("labels2"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                    units::m, Values{1, 1, 3}));
  }
  Dataset d;
};

TEST_F(GroupbyLogicalTest, no_reduction) {
  Dataset expected = copy(d);
  expected.rename(Dim::X, Dim("labels1"));
  expected.setCoord(Dim("labels1"), expected.coords()[Dim("labels1")]);
  expected.coords().erase(Dim("labels2"));
  EXPECT_EQ(groupby(d, Dim("labels1")).all(Dim::X), expected);
  EXPECT_EQ(groupby(d, Dim("labels1")).any(Dim::X), expected);
}

TEST_F(GroupbyLogicalTest, all) {
  Dataset expected;
  expected.setData(
      "a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                              Values{false, false, true, false}));
  expected.setCoord(Dim("labels2"),
                    makeVariable<double>(Dimensions{Dim("labels2"), 2},
                                         units::m, Values{1, 3}));
  EXPECT_EQ(groupby(d, Dim("labels2")).all(Dim::X), expected);
  d["a"].masks().set("mask", makeVariable<bool>(Dimensions{Dim::X, 3},
                                                Values{false, true, false}));
  expected.setData(
      "a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                              Values{true, false, true, false}));
  EXPECT_EQ(groupby(d, Dim("labels2")).all(Dim::X), expected);
}

TEST_F(GroupbyLogicalTest, all_empty_bin) {
  const auto edges = makeVariable<double>(Dimensions{Dim("labels2"), 3},
                                          units::m, Values{0, 1, 3});
  Dataset expected;
  // No contribution in first bin => init to `true`
  expected.setData(
      "a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                              Values{true, false, true, true}));
  expected.setCoord(Dim("labels2"), edges);
  EXPECT_EQ(groupby(d, Dim("labels2"), edges).all(Dim::X), expected);
}

TEST_F(GroupbyLogicalTest, any) {
  Dataset expected;
  expected.setData(
      "a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                              Values{true, false, true, false}));
  expected.setCoord(Dim("labels2"),
                    makeVariable<double>(Dimensions{Dim("labels2"), 2},
                                         units::m, Values{1, 3}));
  EXPECT_EQ(groupby(d, Dim("labels2")).any(Dim::X), expected);
  d["a"].masks().set("mask", makeVariable<bool>(Dimensions{Dim::X, 3},
                                                Values{true, false, false}));
  expected.setData(
      "a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                              Values{false, false, true, false}));
  EXPECT_EQ(groupby(d, Dim("labels2")).any(Dim::X), expected);
}

TEST_F(GroupbyLogicalTest, any_empty_bin) {
  const auto edges = makeVariable<double>(Dimensions{Dim("labels2"), 3},
                                          units::m, Values{0, 1, 3});
  Dataset expected;
  // No contribution in first bin => init to `false`
  expected.setData(
      "a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                              Values{false, true, false, true}));
  expected.setCoord(Dim("labels2"), edges);
  EXPECT_EQ(groupby(d, Dim("labels2"), edges).any(Dim::X), expected);
}

struct GroupbyMinMaxTest : public ::testing::Test {
  GroupbyMinMaxTest() {
    d.setData("a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                        Values{1, 2, 3, 4, 5, 6}));
    d.setCoord(Dim("labels1"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                    units::m, Values{1, 2, 3}));
    d.setCoord(Dim("labels2"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                    units::m, Values{1, 1, 3}));
  }
  Dataset d;
};

TEST_F(GroupbyMinMaxTest, no_reduction) {
  Dataset expected = copy(d);
  expected.rename(Dim::X, Dim("labels1"));
  expected.setCoord(Dim("labels1"), expected.coords()[Dim("labels1")]);
  expected.coords().erase(Dim("labels2"));
  EXPECT_EQ(groupby(d, Dim("labels1")).min(Dim::X), expected);
  EXPECT_EQ(groupby(d, Dim("labels1")).max(Dim::X), expected);
}

TEST_F(GroupbyMinMaxTest, min) {
  Dataset expected;
  expected.setData(
      "a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{1, 3, 4, 6}));
  expected.setCoord(Dim("labels2"),
                    makeVariable<double>(Dimensions{Dim("labels2"), 2},
                                         units::m, Values{1, 3}));
  EXPECT_EQ(groupby(d, Dim("labels2")).min(Dim::X), expected);
  d["a"].masks().set("mask", makeVariable<bool>(Dimensions{Dim::X, 3},
                                                Values{true, false, false}));
  expected.setData(
      "a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{2, 3, 5, 6}));
  EXPECT_EQ(groupby(d, Dim("labels2")).min(Dim::X), expected);
}

TEST_F(GroupbyMinMaxTest, min_empty_bin) {
  const auto edges = makeVariable<double>(Dimensions{Dim("labels2"), 3},
                                          units::m, Values{0, 1, 3});
  Dataset expected;
  const auto max = std::numeric_limits<double>::max();
  expected.setData(
      "a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{max, 1., max, 4.}));
  expected.setCoord(Dim("labels2"), edges);
  EXPECT_EQ(groupby(d, Dim("labels2"), edges).min(Dim::X), expected);
}

TEST_F(GroupbyMinMaxTest, max) {
  Dataset expected;
  expected.setData(
      "a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{2, 3, 5, 6}));
  expected.setCoord(Dim("labels2"),
                    makeVariable<double>(Dimensions{Dim("labels2"), 2},
                                         units::m, Values{1, 3}));
  EXPECT_EQ(groupby(d, Dim("labels2")).max(Dim::X), expected);
  d["a"].masks().set("mask", makeVariable<bool>(Dimensions{Dim::X, 3},
                                                Values{false, true, false}));
  expected.setData(
      "a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{1, 3, 4, 6}));
  EXPECT_EQ(groupby(d, Dim("labels2")).max(Dim::X), expected);
}

TEST_F(GroupbyMinMaxTest, max_empty_bin) {
  const auto edges = makeVariable<double>(Dimensions{Dim("labels2"), 3},
                                          units::m, Values{0, 1, 3});
  Dataset expected;
  const auto lowest = std::numeric_limits<double>::lowest();
  expected.setData(
      "a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{lowest, 2., lowest, 5.}));
  expected.setCoord(Dim("labels2"), edges);
  EXPECT_EQ(groupby(d, Dim("labels2"), edges).max(Dim::X), expected);
}

TEST(GroupbyLargeTest, sum) {
  const scipp::index large = 114688;
  auto data = broadcast(makeVariable<double>(Values{1}),
                        {{Dim::X, Dim::Y}, {large, 10}});
  auto z = makeVariable<int32_t>(Dims{Dim::X}, Shape{large});
  for (scipp::index i = 0; i < large; ++i)
    z.values<int32_t>()[i] = (i / 6000) % 13;
  DataArray da(data);
  da.coords().set(Dim::Z, z);
  auto grouped = groupby(da, Dim::Z).sum(Dim::X);
  EXPECT_EQ(sum(grouped), sum(da));
}
