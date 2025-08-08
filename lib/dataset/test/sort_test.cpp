// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/dataset/sort.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(SortTest, variable_1d) {
  const auto var = makeVariable<float>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                       Values{1, 2, 3}, Variances{4, 5, 6});
  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});
  const auto expected = makeVariable<float>(
      Dims{Dim::X}, Shape{3}, sc_units::m, Values{3, 1, 2}, Variances{6, 4, 5});

  EXPECT_EQ(sort(var, key), expected);
}

TEST(SortTest, variable_1d_descending) {
  const auto var = makeVariable<float>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                       Values{1, 2, 3}, Variances{4, 5, 6});
  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});
  const auto expected = makeVariable<float>(
      Dims{Dim::X}, Shape{3}, sc_units::m, Values{2, 1, 3}, Variances{5, 4, 6});

  EXPECT_EQ(sort(var, key, SortOrder::Descending), expected);
}

TEST(SortTest, variable_1d_bad_key_shape_throws) {
  const auto var = makeVariable<float>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                       Values{1, 2, 3}, Variances{4, 5, 6});
  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{5}, Values{10, 20, -1, 5, 3});
  EXPECT_THROW(sort(var, key), except::DimensionError);
}

TEST(SortTest, variable_2d) {
  const auto var = makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                     sc_units::m, Values{1, 2, 3, 4, 5, 6});

  const auto keyX =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});
  const auto expectedX = makeVariable<int>(
      Dims{Dim::Y, Dim::X}, Shape{2, 3}, sc_units::m, Values{3, 1, 2, 6, 4, 5});

  const auto keyY = makeVariable<int>(Dims{Dim::Y}, Shape{2}, Values{1, 0});
  const auto expectedY = makeVariable<int>(
      Dims{Dim::Y, Dim::X}, Shape{2, 3}, sc_units::m, Values{4, 5, 6, 1, 2, 3});

  EXPECT_EQ(sort(var, keyX), expectedX);
  EXPECT_EQ(sort(var, keyY), expectedY);
}

TEST(SortTest, variable_1d_nan) {
  // We cannot compare the sorted results to reference variables because
  // NaN != Nan. So make sure the non-NaN values are sorted and NaN's are
  // at the beginning / end depending on the sorting order.
  constexpr auto nan = std::numeric_limits<double>::quiet_NaN();
  std::vector test_values{std::vector<double>{4.0, nan, 1.0, 3.0},
                          std::vector<double>{4.0, nan, 1.0, nan},
                          std::vector<double>{4.0, nan, nan, 3.0}};
  std::vector<scipp::index> n_nan{1, 2, 2};
  for (scipp::index i_test = 0; i_test < scipp::size(test_values); ++i_test) {
    const auto &values = test_values.at(i_test);
    const auto n = n_nan.at(i_test);
    const auto var =
        makeVariable<double>(Dims{Dim::X}, Shape{4}, Values(values));

    const auto ascending = sort(var, var, SortOrder::Ascending);
    scipp::index i = 1;
    for (; i < ascending.dims().volume() - n; ++i) {
      ASSERT_LE(ascending.values<double>()[i - 1],
                ascending.values<double>()[i]);
    }
    for (; i < ascending.dims().volume(); ++i) {
      ASSERT_TRUE(std::isnan(ascending.values<double>()[i]));
    }

    const auto descending = sort(var, var, SortOrder::Descending);
    for (i = 0; i < n; ++i) {
      ASSERT_TRUE(std::isnan(descending.values<double>()[i]));
    }
    ++i; // move i - 1 past the last NaN
    for (; i < descending.dims().volume(); ++i) {
      ASSERT_GE(descending.values<double>()[i - 1],
                descending.values<double>()[i]);
    }
  }
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

TEST(SortTest, data_array_bin_edge_coord_throws) {
  Variable data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 3, 2, 4});
  Variable x =
      makeVariable<double>(Dims{Dim::Event}, Shape{5}, Values{5, 3, 2, 4, 1});
  DataArray table = DataArray(data, {{Dim::X, x}});
  EXPECT_THROW(sort(table, Dim::X), except::DimensionError);
}

TEST(SortTest, dataset_1d) {
  Dataset d({{"a", makeVariable<float>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                       Values{1, 2, 3}, Variances{4, 5, 6})},
             {"b", makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::s,
                                        Values{0.1, 0.2, 0.3})}},
            {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                           Values{1, 2, 3})}});

  Dataset expected(
      {{"a", makeVariable<float>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                 Values{3, 1, 2}, Variances{6, 4, 5})},
       {"b", makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::s,
                                  Values{0.3, 0.1, 0.2})}},
      {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                     Values{3, 1, 2})}});

  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});

  EXPECT_EQ(sort(d, key), expected);
}

TEST(SortTest, dataset_1d_descending) {
  Dataset d({{"a", makeVariable<float>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                       Values{1, 2, 3}, Variances{4, 5, 6})},
             {"b", makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::s,
                                        Values{0.1, 0.2, 0.3})}},
            {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                           Values{1, 2, 3})}});

  Dataset expected(
      {{"a", makeVariable<float>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                 Values{2, 1, 3}, Variances{5, 4, 6})},
       {"b", makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::s,
                                  Values{0.2, 0.1, 0.3})}},
      {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                     Values{2, 1, 3})}});

  const auto key =
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{10, 20, -1});

  EXPECT_EQ(sort(d, key, SortOrder::Descending), expected);
}
