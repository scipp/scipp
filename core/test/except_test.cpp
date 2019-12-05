// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <type_traits>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"

using namespace scipp;
using namespace scipp::core;

TEST(StringFormattingTest, to_string_Dataset) {
  Dataset a;
  a.setData("a", makeVariable<double>({}));
  a.setData("b", makeVariable<double>({}));
  // Create new dataset with same variables but different order
  Dataset b;
  b.setData("b", makeVariable<double>({}));
  b.setData("a", makeVariable<double>({}));
  // string representations should be the same
  EXPECT_EQ(to_string(a), to_string(b));
}

std::tuple<Dataset, Dataset> makeDatasets() {
  Dataset a;
  a.setCoord(Dim::X,
             createVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  a.setCoord(Dim::Y,
             createVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3}));
  a.setCoord(Dim::Z,
             createVariable<double>(Dims{Dim::Z}, Shape{3}, Values{1, 2, 3}));
  a.setLabels("label_1",
              createVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23}));
  a.setLabels("label_2",
              createVariable<int>(Dims{Dim::Y}, Shape{3}, Values{21, 22, 23}));
  a.setLabels("label_3",
              createVariable<int>(Dims{Dim::Z}, Shape{3}, Values{21, 22, 23}));
  a.setData("a", createVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  a.setData("b", createVariable<int>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3}));
  a.setData("c", createVariable<int>(Dims{Dim::Z}, Shape{3}, Values{1, 2, 3}));

  Dataset b;
  b.setCoord(Dim::X,
             createVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  b.setCoord(Dim::Y,
             createVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3}));
  b.setCoord(Dim::Z,
             createVariable<double>(Dims{Dim::Z}, Shape{3}, Values{1, 2, 3}));
  b.setLabels("label_1",
              createVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23}));
  b.setLabels("label_2",
              createVariable<int>(Dims{Dim::Y}, Shape{3}, Values{21, 22, 23}));
  b.setLabels("label_3",
              createVariable<int>(Dims{Dim::Z}, Shape{3}, Values{21, 22, 23}));
  b.setData("a", createVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  b.setData("b", createVariable<int>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3}));
  b.setData("c", createVariable<int>(Dims{Dim::Z}, Shape{3}, Values{1, 2, 3}));

  return std::make_tuple(a, b);
}

TEST(StringFormattingTest, to_string_MutableProxy) {
  auto [a, b] = makeDatasets();

  EXPECT_EQ(to_string(a.coords()), to_string(b.coords()));
  EXPECT_EQ(to_string(a.labels()), to_string(b.labels()));
  EXPECT_EQ(to_string(a.attrs()), to_string(b.attrs()));
}

TEST(StringFormattingTest, to_string_ConstProxy) {
  const auto [a, b] = makeDatasets();

  EXPECT_EQ(to_string(a.coords()), to_string(b.coords()));
  EXPECT_EQ(to_string(a.labels()), to_string(b.labels()));
  EXPECT_EQ(to_string(a.attrs()), to_string(b.attrs()));
}

TEST(StringFormattingTest, to_string_sparse_Dataset) {
  Dataset a;
  a.setSparseCoord(
      "a", makeVariable<double>({Dim::Y, Dim::X}, {4, Dimensions::Sparse}));
  ASSERT_NO_THROW(to_string(a));
}

TEST(ValidSliceTest, test_slice_range) {
  Dimensions dims{Dim::X, 3};
  EXPECT_NO_THROW(expect::validSlice(dims, Slice(Dim::X, 0)));
  EXPECT_NO_THROW(expect::validSlice(dims, Slice(Dim::X, 2)));
  EXPECT_NO_THROW(expect::validSlice(dims, Slice(Dim::X, 0, 3)));
  EXPECT_THROW(expect::validSlice(dims, Slice(Dim::X, 3)), except::SliceError);
  EXPECT_THROW(expect::validSlice(dims, Slice(Dim::X, -1)), except::SliceError);
  EXPECT_THROW(expect::validSlice(dims, Slice(Dim::X, 0, 4)),
               except::SliceError);
}

TEST(ValidSliceTest, test_dimension_contained) {
  Dimensions dims{{Dim::X, 3}, {Dim::Z, 3}};
  EXPECT_NO_THROW(expect::validSlice(dims, Slice(Dim::X, 0)));
  EXPECT_THROW(expect::validSlice(dims, Slice(Dim::Y, 0)), except::SliceError);
}
