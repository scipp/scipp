// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/mean.h"
#include "scipp/dataset/shape.h"
#include "scipp/dataset/sum.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/shape.h"

#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

struct GroupbyTest : public ::testing::Test {
  GroupbyTest()
      : d({{"a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                      sc_units::m, Values{1, 2, 3, 1, 2, 3},
                                      Variances{4, 5, 6, 4, 5, 6})},
           {"c", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                      sc_units::s, Values{1, 2, 3, 4, 5, 6})}},
          {{Dim("labels1"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                 sc_units::m, Values{1, 2, 3})},
           {Dim("labels2"),
            makeVariable<double>(Dimensions{Dim::X, 3}, sc_units::m,
                                 Values{1, 1, 3})}}) {}

  Dataset d;
};

TEST_F(GroupbyTest, fail_key_not_found) {
  EXPECT_THROW(groupby(d, Dim("invalid")), except::NotFoundError);
  EXPECT_THROW(groupby(d["a"], Dim("invalid")), except::NotFoundError);
}

TEST_F(GroupbyTest, fail_key_2d) {
  d.setCoord(Dim("2d"),
             makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                  sc_units::s, Values{1, 2, 3, 4, 5, 6}));
  EXPECT_THROW(groupby(d, Dim("2d")), except::DimensionError);
  EXPECT_THROW(groupby(d["a"], Dim("2d")), except::DimensionError);
}

TEST_F(GroupbyTest, fail_key_with_variances) {
  d.setCoord(Dim("variances"),
             makeVariable<double>(Dimensions{Dim::X, 3}, sc_units::m,
                                  Values{1, 2, 3}, Variances{4, 5, 6}));
  EXPECT_THROW(groupby(d, Dim("variances")), except::VariancesError);
  EXPECT_THROW(groupby(d["a"], Dim("variances")), except::VariancesError);
}

TEST_F(GroupbyTest, drop_2d_coord) {
  d.setCoord(Dim("2d"), makeVariable<float>(Dims{Dim::X, Dim::Z}, Shape{3, 2}));
  EXPECT_NO_THROW(groupby(d, Dim("labels2")));
  EXPECT_FALSE(
      groupby(d, Dim("labels2")).sum(Dim::X).coords().contains(Dim("2d")));
}

TEST_F(GroupbyTest, dataset_1d_and_2d) {
  Dim dim("labels2");
  Dataset expected(
      {{"a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2}, sc_units::m,
                                  Values{1.5, 3.0, 1.5, 3.0},
                                  Variances{9.0 / 4, 6.0, 9.0 / 4, 6.0})},
       {"c", makeVariable<double>(Dims{Dim(Dim::Z), dim}, Shape{2, 2},
                                  sc_units::s, Values{1.5, 3.0, 4.5, 6.0})}});
  expected.setCoord(dim, makeVariable<double>(Dims{dim}, Shape{2}, sc_units::m,
                                              Values{1, 3}));

  EXPECT_EQ(groupby(d, dim).mean(Dim::X), expected);
  EXPECT_EQ(groupby(d["a"], dim).mean(Dim::X), expected["a"]);
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

struct GroupbyReductionTest : public ::testing::Test {
  GroupbyReductionTest()
      : d({{"a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                      sc_units::m, Values{1, 2, 3, 1, 2, 3})},
           {"c",
            makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                 sc_units::s, Values{1, 2, 3, 4, 5, 6})}}) {
    d.setCoord(Dim("labels1"),
               makeVariable<double>(Dimensions{Dim::X, 3}, sc_units::m,
                                    Values{1, 2, 3}));
    d.setCoord(Dim("labels2"),
               makeVariable<double>(Dimensions{Dim::X, 3}, sc_units::m,
                                    Values{1, 1, 3}));
  }

  Dataset d;
};

TEST_F(GroupbyReductionTest, sum_size_1_groups) {
  const Dim dim("labels1");
  auto expected = copy(d).rename_dims({{Dim::X, dim}});
  expected.coords().erase(Dim("labels2"));
  EXPECT_EQ(groupby(d, dim).sum(Dim::X), expected);
}

TEST_F(GroupbyReductionTest, sum_groups_with_multiple_elements) {
  const Dim dim("labels2");
  Dataset expected(
      {{"a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2}, sc_units::m,
                                  Values{3, 3, 3, 3})},
       {"c", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2}, sc_units::s,
                                  Values{3, 3, 9, 6})}});
  expected.setCoord(dim, makeVariable<double>(Dims{dim}, sc_units::m, Shape{2},
                                              Values{1, 3}));
  EXPECT_EQ(groupby(d, dim).sum(Dim::X), expected);
}

TEST_F(GroupbyReductionTest, max_size_1_groups) {
  const Dim dim("labels1");
  auto expected = copy(d).rename_dims({{Dim::X, dim}});
  expected.coords().erase(Dim("labels2"));
  EXPECT_EQ(groupby(d, dim).max(Dim::X), expected);
}

TEST_F(GroupbyReductionTest, max_groups_with_multiple_elements) {
  const Dim dim("labels2");
  Dataset expected(
      {{"a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2}, sc_units::m,
                                  Values{2, 3, 2, 3})},
       {"c", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2}, sc_units::s,
                                  Values{2, 3, 5, 6})}});
  expected.setCoord(dim, makeVariable<double>(Dims{dim}, sc_units::m, Shape{2},
                                              Values{1, 3}));
  EXPECT_EQ(groupby(d, dim).max(Dim::X), expected);
}

struct GroupbyReductionMultipleSubgroupsTest : public ::testing::Test {
  GroupbyReductionMultipleSubgroupsTest()
      : da{makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 4}},
                                sc_units::m, Values{1, 2, 3, 4, 5, 6, 7, 8}),
           {{Dim("labels"),
             makeVariable<double>(Dimensions{Dim::X, 4}, sc_units::m,
                                  Values{1, 1, 3, 1})}}} {}

  DataArray da;
};

TEST_F(GroupbyReductionMultipleSubgroupsTest, sum) {
  const Dim dim("labels");
  DataArray expected{makeVariable<double>(Dimensions{{Dim::Z, 2}, {dim, 2}},
                                          sc_units::m, Values{7, 3, 19, 7}),
                     {{dim, makeVariable<double>(Dimensions{dim, 2},
                                                 sc_units::m, Values{1, 3})}}};
  EXPECT_EQ(groupby(da, dim).sum(Dim::X), expected);
}

TEST_F(GroupbyReductionMultipleSubgroupsTest, max) {
  const Dim dim("labels");
  DataArray expected{makeVariable<double>(Dimensions{{Dim::Z, 2}, {dim, 2}},
                                          sc_units::m, Values{4, 3, 8, 7}),
                     {{dim, makeVariable<double>(Dimensions{dim, 2},
                                                 sc_units::m, Values{1, 3})}}};
  EXPECT_EQ(groupby(da, dim).max(Dim::X), expected);
}

struct GroupbyMaskedTest : public GroupbyTest {
  GroupbyMaskedTest() : GroupbyTest() {
    for (const auto &item : {"a", "c"})
      d[item].masks().set("mask_x",
                          makeVariable<bool>(Dimensions{Dim::X, 3},
                                             Values{false, true, false}));
    for (const auto &item : {"a", "c"})
      d[item].masks().set("mask_z", makeVariable<bool>(Dimensions{Dim::Z, 2},
                                                       Values{false, true}));
  }
};

TEST_F(GroupbyMaskedTest, sum) {
  const Dim dim("labels2");
  Dataset expected(
      {{"a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2}, sc_units::m,
                                  Values{1, 3, 1, 3}, Variances{4, 6, 4, 6})},
       {"c", makeVariable<double>(Dimensions{{Dim::Z, 2}, {dim, 2}},
                                  sc_units::s, Values{1, 3, 4, 6})}},
      {{dim,
        makeVariable<double>(Dimensions{dim, 2}, sc_units::m, Values{1, 3})}});
  for (const auto &item : {"a", "c"})
    expected[item].masks().set(
        "mask_z",
        makeVariable<bool>(Dimensions{Dim::Z, 2}, Values{false, true}));

  const auto result = groupby(d, dim).sum(Dim::X);
  EXPECT_EQ(result, expected);
  // Ensure reduction operation does NOT share the unrelated mask
  result["a"].masks()["mask_z"] |= true * sc_units::none;
  EXPECT_NE(result["a"].masks()["mask_z"], d["a"].masks()["mask_z"]);
}

TEST_F(GroupbyMaskedTest, sum_irrelevant_mask) {
  const Dim dim("labels2");
  Dataset expected(
      {{"a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2}, sc_units::m,
                                  Values{3, 3, 3, 3}, Variances{9, 6, 9, 6})},
       {"c", makeVariable<double>(Dimensions{{Dim::Z, 2}, {dim, 2}},
                                  sc_units::s, Values{3, 3, 9, 6})}},
      {{dim,
        makeVariable<double>(Dimensions{dim, 2}, sc_units::m, Values{1, 3})}});
  for (const auto &item : {"a", "c"})
    expected[item].masks().set(
        "mask_z",
        makeVariable<bool>(Dimensions{Dim::Z, 2}, Values{false, true}));

  for (const auto &item : {"a", "c"})
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
  const Dim dim("labels2");
  Dataset expected(
      {{"a", makeVariable<double>(Dims{Dim::Z, dim}, Shape{2, 2}, sc_units::m,
                                  Values{1, 3, 1, 3}, Variances{4, 6, 4, 6})},
       {"c", makeVariable<double>(Dimensions{{Dim::Z, 2}, {dim, 2}},
                                  sc_units::s, Values{1, 3, 4, 6})}},
      {{dim,
        makeVariable<double>(Dimensions{dim, 2}, sc_units::m, Values{1, 3})}});
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

  EXPECT_EQ(result["c"].template values<double>()[0], 1.0);
  EXPECT_TRUE(std::isnan(result["c"].template values<double>()[1]));
  EXPECT_EQ(result["c"].template values<double>()[2], 3.0);
  EXPECT_EQ(result["c"].template values<double>()[3], 4.0);
  EXPECT_TRUE(std::isnan(result["c"].template values<double>()[4]));
  EXPECT_EQ(result["c"].template values<double>()[5], 6.0);
}

TEST_F(GroupbyMaskedTest, mean2) {
  for (const auto &item : {"a", "c"})
    d[item].masks().set(
        "mask_x",
        makeVariable<bool>(Dimensions{Dim::X, 3}, Values{false, false, true}));

  const Dim dim("labels2");
  const auto result = groupby(d, dim).mean(Dim::X);

  EXPECT_EQ(result["a"].template values<double>()[0], 1.5);
  EXPECT_TRUE(std::isnan(result["a"].template values<double>()[1]));
  EXPECT_EQ(result["a"].template variances<double>()[0], 2.25);
  EXPECT_TRUE(std::isnan(result["a"].template variances<double>()[1]));

  EXPECT_EQ(result["c"].template values<double>()[0], 1.5);
  EXPECT_TRUE(std::isnan(result["c"].template values<double>()[1]));
  EXPECT_EQ(result["c"].template values<double>()[2], 4.5);
  EXPECT_TRUE(std::isnan(result["c"].template values<double>()[3]));

  EXPECT_EQ(
      result.coords()[dim],
      makeVariable<double>(Dimensions{dim, 2}, sc_units::m, Values{1.0, 3.0}));
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
  GroupbyWithBinsTest()
      : d({{"a", makeVariable<double>(Dimensions{Dim::X, 5}, sc_units::s,
                                      Values{0.1, 0.2, 0.3, 0.4, 0.5})}}) {
    d.setCoord(Dim("labels1"),
               makeVariable<double>(Dimensions{Dim::X, 5}, sc_units::m,
                                    Values{1, 2, 3, 4, 5}));
    d.setCoord(Dim("labels2"),
               makeVariable<double>(Dimensions{Dim::X, 5}, sc_units::m,
                                    Values{1.0, 1.1, 2.5, 4.0, 1.2}));
  }

  Dataset d;
};

TEST_F(GroupbyWithBinsTest, bins) {
  auto bins = makeVariable<double>(Dims{Dim::Z}, Shape{4}, sc_units::m,
                                   Values{0.0, 1.0, 2.0, 3.0});

  Dataset expected(
      {{"a", makeVariable<double>(Dims{Dim::Z}, Shape{3}, sc_units::s,
                                  Values{0.0, 0.8, 0.3})}},
      {{Dim::Z, bins}});

  EXPECT_EQ(groupby(d, Dim("labels2"), bins).sum(Dim::X), expected);
  EXPECT_EQ(groupby(d["a"], Dim("labels2"), bins).sum(Dim::X), expected["a"]);
}

TEST_F(GroupbyWithBinsTest, bins_mean_empty) {
  auto bins = makeVariable<double>(Dims{Dim::Z}, Shape{4}, sc_units::m,
                                   Values{0.0, 1.0, 2.0, 3.0});

  const auto binned = groupby(d, Dim("labels2"), bins).mean(Dim::X);
  EXPECT_TRUE(std::isnan(binned["a"].values<double>()[0]));
  EXPECT_FALSE(std::isnan(binned["a"].values<double>()[1]));
}

TEST_F(GroupbyWithBinsTest, single_bin) {
  auto bins = makeVariable<double>(Dims{Dim::Z}, Shape{2}, sc_units::m,
                                   Values{1.0, 5.0});
  const auto groups = groupby(d, Dim("labels2"), bins);

  // Non-range slice makes Dim::Z unaligned, so the result must be equal to a
  // global `sum` or `mean` with the corresponding coord (the edges) added.
  const auto add_bins = [&bins](auto data) {
    data.coords().set(Dim("z"), bins);
    data.coords().set_aligned(Dim("z"), false);
    return data;
  };
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 0}), add_bins(sum(d, Dim::X)));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 0}), add_bins(mean(d, Dim::X)));
}

TEST_F(GroupbyWithBinsTest, two_bins) {
  auto bins = makeVariable<double>(Dims{Dim::Z}, Shape{3}, sc_units::m,
                                   Values{1.0, 2.0, 5.0});
  const auto groups = groupby(d, Dim("labels2"), bins);

  const auto add_bins = [&bins](auto data, const scipp::index bin) {
    data.coords().set(Dim("z"), bins.slice({Dim::Z, bin, bin + 2}));
    data.coords().set_aligned(Dim("z"), false);
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
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, sc_units::Unit(sc_units::m),
                           Values{0.0, 1.0, 2.0, 3.0});

  auto const var =
      makeVariable<double>(Dimensions{Dim::X, 5}, sc_units::Unit(sc_units::m),
                           Values{1.0, 1.1, 2.5, 4.0, 1.2});

  d.setCoord(Dim("labels2"), var);

  auto const groupby_label = groupby(d, Dim("labels2"), bins);
  auto const groupby_variable = groupby(d, var, bins);

  EXPECT_EQ(groupby_label.key(), groupby_variable.key());

  auto const var_bad = makeVariable<double>(
      Dimensions{Dim::X, 1}, sc_units::Unit(sc_units::m), Values{1.0});

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
       {Dim("labels"), makeVariable<int64_t>(Dims{Dim::Y}, Shape{3},
                                             sc_units::m, Values{1, 1, 3})}},
      {}};

  DataArray expected{
      make_events_out(),
      {{Dim("0-d"), makeVariable<double>(Values{1.2})},
       {Dim("labels"), makeVariable<int64_t>(Dims{Dim("labels")}, Shape{2},
                                             sc_units::m, Values{1, 3})}},
      {}};
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
  a.coords().set(Dim::X, makeVariable<double>(
                             Dims{Dim::Y, Dim::X}, Shape{3, 3}, sc_units::m,
                             Values{1, 3, 8, 1, 3, 9, 1, 3, 10}));
  auto grouped = groupby(a, Dim("labels")).concat(Dim::Y);
  EXPECT_EQ(
      grouped.coords().extract(Dim::X),
      makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m, Values{1, 10}));
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
       {Dim("labels"), makeVariable<double>(Dims{Dim::Y}, Shape{3}, sc_units::m,
                                            Values{1, 1, 3})}},
      {{"mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{3},
                                     Values{false, true, false})}}};
  const DataArray expected{
      make_events_out(true),
      {{Dim("labels"), makeVariable<double>(Dims{Dim("labels")}, Shape{2},
                                            sc_units::m, Values{1, 3})}}};
};

TEST_F(GroupbyBinnedMaskTest, concatenate) {
  EXPECT_EQ(groupby(a, Dim("labels")).concat(Dim::Y), expected);
}

struct GroupbyLogicalTest : public ::testing::Test {
  GroupbyLogicalTest()
      : d({{"a",
            makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                               Values{true, false, false, true, true, false})}},
          {{Dim("labels1"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                 sc_units::m, Values{1, 2, 3})},
           {Dim("labels2"),
            makeVariable<double>(Dimensions{Dim::X, 3}, sc_units::m,
                                 Values{1, 1, 3})}}) {}
  Dataset d;
};

TEST_F(GroupbyLogicalTest, no_reduction) {
  auto expected = copy(d).rename_dims({{Dim::X, Dim("labels1")}});
  expected.setCoord(Dim("labels1"), expected.coords()[Dim("labels1")]);
  expected.coords().erase(Dim("labels2"));
  EXPECT_EQ(groupby(d, Dim("labels1")).all(Dim::X), expected);
  EXPECT_EQ(groupby(d, Dim("labels1")).any(Dim::X), expected);
}

TEST_F(GroupbyLogicalTest, all) {
  Dataset expected(
      {{"a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{false, false, true, false})}},
      {{Dim("labels2"), makeVariable<double>(Dimensions{Dim("labels2"), 2},
                                             sc_units::m, Values{1, 3})}});
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
                                          sc_units::m, Values{0, 1, 3});
  // No contribution in first bin => init to `true`
  Dataset expected(
      {{"a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{true, false, true, true})}},
      {{Dim("labels2"), edges}});
  EXPECT_EQ(groupby(d, Dim("labels2"), edges).all(Dim::X), expected);
}

TEST_F(GroupbyLogicalTest, any) {
  Dataset expected(
      {{"a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{true, false, true, false})}},
      {{Dim("labels2"), makeVariable<double>(Dimensions{Dim("labels2"), 2},
                                             sc_units::m, Values{1, 3})}});
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
                                          sc_units::m, Values{0, 1, 3});
  // No contribution in first bin => init to `false`
  Dataset expected(
      {{"a", makeVariable<bool>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                Values{false, true, false, true})}},
      {{Dim("labels2"), edges}});
  EXPECT_EQ(groupby(d, Dim("labels2"), edges).any(Dim::X), expected);
}

struct GroupbyMinMaxTest : public ::testing::Test {
  GroupbyMinMaxTest()
      : d({{"a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim::X, 3}},
                                      Values{1, 2, 3, 4, 5, 6})}},
          {{Dim("labels1"), makeVariable<double>(Dimensions{Dim::X, 3},
                                                 sc_units::m, Values{1, 2, 3})},
           {Dim("labels2"),
            makeVariable<double>(Dimensions{Dim::X, 3}, sc_units::m,
                                 Values{1, 1, 3})}}) {}
  Dataset d;
};

TEST_F(GroupbyMinMaxTest, no_reduction) {
  auto expected = copy(d).rename_dims({{Dim::X, Dim("labels1")}});
  expected.setCoord(Dim("labels1"), expected.coords()[Dim("labels1")]);
  expected.coords().erase(Dim("labels2"));
  EXPECT_EQ(groupby(d, Dim("labels1")).min(Dim::X), expected);
  EXPECT_EQ(groupby(d, Dim("labels1")).max(Dim::X), expected);
}

TEST_F(GroupbyMinMaxTest, min) {
  Dataset expected(
      {{"a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                  Values{1, 3, 4, 6})}},
      {{Dim("labels2"), makeVariable<double>(Dimensions{Dim("labels2"), 2},
                                             sc_units::m, Values{1, 3})}});
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
                                          sc_units::m, Values{0, 1, 3});
  const auto max = std::numeric_limits<double>::max();
  Dataset expected(
      {{"a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                  Values{max, 1., max, 4.})}},
      {{Dim("labels2"), edges}});
  EXPECT_EQ(groupby(d, Dim("labels2"), edges).min(Dim::X), expected);
}

TEST_F(GroupbyMinMaxTest, max) {
  Dataset expected(
      {{"a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                  Values{2, 3, 5, 6})}},
      {{Dim("labels2"), makeVariable<double>(Dimensions{Dim("labels2"), 2},
                                             sc_units::m, Values{1, 3})}});
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
                                          sc_units::m, Values{0, 1, 3});
  const auto lowest = std::numeric_limits<double>::lowest();
  Dataset expected(
      {{"a", makeVariable<double>(Dimensions{{Dim::Z, 2}, {Dim("labels2"), 2}},
                                  Values{lowest, 2., lowest, 5.})}},
      {{Dim("labels2"), edges}});
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

TEST_F(GroupbyWithBinsTest, groupby_reference_prereserved) {
  auto bins = makeVariable<double>(Dims{Dim::Z}, Shape{4}, sc_units::m,
                                   Values{0.0, 1.0, 2.0, 3.0});

  auto grouped = groupby(d, Dim("labels2"), bins);
  EXPECT_EQ(&grouped.sum(Dim::X).coords()[Dim::Z].values<double>()[0],
            &bins.values<double>()[0]);
}
