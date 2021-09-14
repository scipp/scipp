// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

using namespace scipp;
using namespace scipp::dataset;

class BinsViewTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable var = make_bins(indices, Dim::X, buffer);
};

TEST_F(BinsViewTest, erase) {
  auto view = bins_view<DataArray>(var);
  auto da = var.bin_buffer<DataArray>();
  EXPECT_TRUE(da.coords().contains(Dim::X));
  view.coords().erase(Dim::X);
  EXPECT_FALSE(da.coords().contains(Dim::X));
}

TEST_F(BinsViewTest, insert) {
  auto view = bins_view<DataArray>(var);
  const auto &da = var.bin_buffer<DataArray>();
  EXPECT_FALSE(da.coords().contains(Dim::Y));
  view.coords().set(Dim::Y, view.coords()[Dim::X]);
  EXPECT_TRUE(da.coords().contains(Dim::Y));
}

TEST_F(BinsViewTest, slice_readonly) {
  auto slice = var.slice({Dim::Y, 0});
  auto view = bins_view<DataArray>(slice);
  ASSERT_THROW(view.coords().erase(Dim::X), except::DataArrayError);
  auto buf = slice.bin_buffer<DataArray>();
  ASSERT_THROW(buf.coords().erase(Dim::X), except::DataArrayError);
  EXPECT_TRUE(buf.is_readonly());
  EXPECT_TRUE(buf.coords().is_readonly());
  EXPECT_TRUE(buf.masks().is_readonly());
  EXPECT_TRUE(buf.attrs().is_readonly());
  EXPECT_TRUE(buf.meta().is_readonly());
  auto copied(buf); // Shallow copy clears flags, as usual
  EXPECT_FALSE(copied.is_readonly());
  EXPECT_FALSE(copied.coords().is_readonly());
  EXPECT_TRUE(copied.coords()[Dim::X].is_readonly());
}

TEST_F(BinsViewTest, constituents_erase) {
  auto &&[i, dim, buf] = var.constituents<DataArray>();
  auto da = var.bin_buffer<DataArray>();
  EXPECT_TRUE(da.coords().contains(Dim::X));
  buf.coords().erase(Dim::X);
  // Constituents returns (shallow) copy, not modified.
  EXPECT_TRUE(da.coords().contains(Dim::X));
  EXPECT_TRUE(
      std::get<2>(var.constituents<DataArray>()).coords().contains(Dim::X));
}

TEST_F(BinsViewTest, constituents_insert) {
  auto &&[i, dim, buf] = var.constituents<DataArray>();
  auto da = var.bin_buffer<DataArray>();
  EXPECT_FALSE(da.coords().contains(Dim::Y));
  buf.coords().set(Dim::Y, buf.coords()[Dim::X]);
  // Constituents returns (shallow) copy, not modified.
  EXPECT_FALSE(da.coords().contains(Dim::Y));
  EXPECT_FALSE(
      std::get<2>(var.constituents<DataArray>()).coords().contains(Dim::Y));
}
