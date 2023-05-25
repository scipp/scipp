// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/arithmetic.h"

#include "dataset_test_common.h"
#include "test_macros.h"

using namespace scipp;

TEST(DataArrayArithmeticTest, fail_op_non_matching_aligned_coords) {
  auto coord_1 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  auto coord_2 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  const DataArray da_1(data, {{Dim::X, coord_1}, {Dim::Y, data}});
  const DataArray da_2(data, {{Dim::X, coord_2}, {Dim::Y, data}});
  EXPECT_THROW_DISCARD(da_1 + da_2, except::CoordMismatchError);
  EXPECT_THROW_DISCARD(da_1 - da_2, except::CoordMismatchError);
}

TEST(DataArrayArithmeticTest, aligned_coord_overrides_unaligned) {
  auto coord_1 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  auto coord_2 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  coord_2.set_aligned(false);
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  const DataArray da_1(data, {{Dim::X, coord_1}});
  const DataArray da_2(data, {{Dim::X, coord_2}});

  const auto res = da_1 + da_2;
  EXPECT_EQ(res.coords()[Dim::X], coord_1);
  EXPECT_TRUE(res.coords()[Dim::X].is_aligned());
}

TEST(DataArrayArithmeticTest, merge_coords_alignment) {
  const auto coord_1 =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  auto coord_2 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  auto coord_3 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  auto coord_4 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  auto coord_5 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  const auto data =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});

  coord_3.set_aligned(false);
  coord_5.set_aligned(false);
  const DataArray da_1(data, {{Dim::X, coord_1},
                              {Dim::Y, copy(coord_2)},
                              {Dim::Z, copy(coord_3)},
                              {Dim{"1.4"}, coord_4},
                              {Dim{"1.5"}, coord_5}});

  coord_2.set_aligned(false);
  coord_3.set_aligned(true);
  const DataArray da_2(data, {{Dim::X, coord_1},
                              {Dim::Y, coord_2},
                              {Dim::Z, coord_3},
                              {Dim{"2.4"}, coord_4},
                              {Dim{"2.5"}, coord_5}});

  const auto res = da_1 + da_2;
  EXPECT_TRUE(res.coords()[Dim::X].is_aligned());
  EXPECT_TRUE(res.coords()[Dim::Y].is_aligned());
  EXPECT_TRUE(res.coords()[Dim::Z].is_aligned());
  EXPECT_TRUE(res.coords()[Dim{"1.4"}].is_aligned());
  EXPECT_FALSE(res.coords()[Dim{"1.5"}].is_aligned());
  EXPECT_TRUE(res.coords()[Dim{"2.4"}].is_aligned());
  EXPECT_FALSE(res.coords()[Dim{"2.5"}].is_aligned());
}

TEST(DataArrayArithmeticTest, operation_does_not_overwrite_input_alignment) {
  const auto coord_1 =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  auto coord_2 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  auto coord_3 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  const auto data =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});

  coord_3.set_aligned(false);
  DataArray da_1(
      data,
      {{Dim::X, coord_1}, {Dim::Y, copy(coord_2)}, {Dim::Z, copy(coord_3)}});

  coord_2.set_aligned(false);
  coord_3.set_aligned(true);
  DataArray da_2(data,
                 {{Dim::X, coord_1}, {Dim::Y, coord_2}, {Dim::Z, coord_3}});

  [[maybe_unused]] const auto res = da_1 + da_2;
  EXPECT_TRUE(da_1.coords()[Dim::X].is_aligned());
  EXPECT_TRUE(da_1.coords()[Dim::Y].is_aligned());
  EXPECT_FALSE(da_1.coords()[Dim::Z].is_aligned());
  EXPECT_TRUE(da_2.coords()[Dim::X].is_aligned());
  EXPECT_FALSE(da_2.coords()[Dim::Y].is_aligned());
  EXPECT_TRUE(da_2.coords()[Dim::Z].is_aligned());
}

TEST(DataArrayArithmeticTest, sum_dataset_columns_via_DataArray) {
  DatasetFactory3D factory;
  auto dataset = factory.make();

  DataArray array(dataset["data_zyx"]);
  auto sum = array + dataset["data_xyz"];

  dataset["data_zyx"] += dataset["data_xyz"];

  // This would fail if the data items had attributes, since += preserves them
  // but + does not.
  EXPECT_EQ(sum, dataset["data_zyx"]);
}
