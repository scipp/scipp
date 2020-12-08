// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/dataset/sort.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(SortTest, variable_1d) {
  const auto var = makeVariable<float>(Dims{Dim::X}, Shape{3}, units::m,
                                       Values{1, 2, 3}, Variances{4, 5, 6});
  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});
  const auto expected = makeVariable<float>(
      Dims{Dim::X}, Shape{3}, units::m, Values{3, 1, 2}, Variances{6, 4, 5});

  EXPECT_EQ(sort(var, key), expected);
}

TEST(SortTest, variable_1d_descending) {
  const auto var = makeVariable<float>(Dims{Dim::X}, Shape{3}, units::m,
                                       Values{1, 2, 3}, Variances{4, 5, 6});
  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});
  const auto expected = makeVariable<float>(
      Dims{Dim::X}, Shape{3}, units::m, Values{2, 1, 3}, Variances{5, 4, 6});

  EXPECT_EQ(sort(var, key, SortOrder::Descending), expected);
}

TEST(SortTest, variable_2d) {
  const auto var = makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                     units::m, Values{1, 2, 3, 4, 5, 6});

  const auto keyX =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});
  const auto expectedX = makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                           units::m, Values{3, 1, 2, 6, 4, 5});

  const auto keyY = makeVariable<int>(Dims{Dim::Y}, Shape{2}, Values{1, 0});
  const auto expectedY = makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                           units::m, Values{4, 5, 6, 1, 2, 3});

  EXPECT_EQ(sort(var, keyX), expectedX);
  EXPECT_EQ(sort(var, keyY), expectedY);
}

TEST(SortTest, data_array_1d) {
  Variable data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 3, 2, 4});
  Variable x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{3, 2, 4, 1});
  Variable mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                     Values{true, false, false, false});
  Variable scalar = makeVariable<double>(Values{1.1});
  DataArray table =
      DataArray(data, {{Dim::X, x}, {Dim("scalar"), scalar}}, {{"mask", mask}});
  Variable edges_x =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 2, 4});
  Variable sorted_data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{4, 2, 1, 3}, Variances{4, 3, 1, 2});
  Variable sorted_x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4});
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                            Values{false, false, true, false});
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}, {Dim("scalar"), scalar}},
                {{"mask", sorted_mask}});
  EXPECT_EQ(sort(table, Dim::X), sorted_table);
}

TEST(SortTest, data_array_1d_descending) {
  Variable data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 3, 2, 4});
  Variable x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{3, 2, 4, 1});
  Variable mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                     Values{true, false, false, false});
  Variable scalar = makeVariable<double>(Values{1.1});
  DataArray table =
      DataArray(data, {{Dim::X, x}, {Dim("scalar"), scalar}}, {{"mask", mask}});
  Variable edges_x =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 2, 4});
  Variable sorted_data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{3, 1, 2, 4}, Variances{2, 1, 3, 4});
  Variable sorted_x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{4, 3, 2, 1});
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                            Values{false, true, false, false});
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}, {Dim("scalar"), scalar}},
                {{"mask", sorted_mask}});
  EXPECT_EQ(sort(table, Dim::X, SortOrder::Descending), sorted_table);
}

TEST(SortTest, dataset_1d) {
  Dataset d;
  d.setData("a", makeVariable<float>(Dims{Dim::X}, Shape{3}, units::m,
                                     Values{1, 2, 3}, Variances{4, 5, 6}));
  d.setData("b", makeVariable<double>(Dims{Dim::X}, Shape{3}, units::s,
                                      Values{0.1, 0.2, 0.3}));
  d.setData("scalar", makeVariable<double>(Values{1.2}));
  d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, units::m,
                                          Values{1, 2, 3}));

  Dataset expected;
  expected.setData("a",
                   makeVariable<float>(Dims{Dim::X}, Shape{3}, units::m,
                                       Values{3, 1, 2}, Variances{6, 4, 5}));
  expected.setData("b", makeVariable<double>(Dims{Dim::X}, Shape{3}, units::s,
                                             Values{0.3, 0.1, 0.2}));
  expected.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3},
                                                 units::m, Values{3, 1, 2}));

  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});

  // Note that the result does not contain `scalar`. Is this a bug or a feature?
  // - Should we throw if there is any scalar data/coord?
  // - Should we preserve scalars?
  EXPECT_EQ(sort(d, key), expected);
}

TEST(SortTest, dataset_1d_descending) {
  Dataset d;
  d.setData("a", makeVariable<float>(Dims{Dim::X}, Shape{3}, units::m,
                                     Values{1, 2, 3}, Variances{4, 5, 6}));
  d.setData("b", makeVariable<double>(Dims{Dim::X}, Shape{3}, units::s,
                                      Values{0.1, 0.2, 0.3}));
  d.setData("scalar", makeVariable<double>(Values{1.2}));
  d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, units::m,
                                          Values{1, 2, 3}));

  Dataset expected;
  expected.setData("a",
                   makeVariable<float>(Dims{Dim::X}, Shape{3}, units::m,
                                       Values{2, 1, 3}, Variances{5, 4, 6}));
  expected.setData("b", makeVariable<double>(Dims{Dim::X}, Shape{3}, units::s,
                                             Values{0.2, 0.1, 0.3}));
  expected.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3},
                                                 units::m, Values{2, 1, 3}));

  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});

  // Note that the result does not contain `scalar`. Is this a bug or a feature?
  // - Should we throw if there is any scalar data/coord?
  // - Should we preserve scalars?
  EXPECT_EQ(sort(d, key, SortOrder::Descending), expected);
}
