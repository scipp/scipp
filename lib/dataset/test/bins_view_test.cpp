// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
  EXPECT_TRUE(bins_view<DataArray>(var).coords().contains(Dim::X));
  view.coords().erase(Dim::X);
  EXPECT_FALSE(bins_view<DataArray>(var).coords().contains(Dim::X));
}

TEST_F(BinsViewTest, insert) {
  auto view = bins_view<DataArray>(var);
  EXPECT_FALSE(bins_view<DataArray>(var).coords().contains(Dim::Y));
  view.coords().set(Dim::Y, view.coords()[Dim::X]);
  EXPECT_TRUE(bins_view<DataArray>(var).coords().contains(Dim::Y));
}

// TODO Currently disabled until we have proper handling of readonly flags when
// slicing binned variables. We need to set a readonly flag on the buffer, but
// not on the buffer columns, such that insert/erase of columns is prohibited
// but value modification is supported.
TEST_F(BinsViewTest, DISABLED_slice_readonly) {
  auto slice = var.slice({Dim::Y, 0});
  auto view = bins_view<DataArray>(slice);
  EXPECT_THROW(view.coords().erase(Dim::X), except::DataArrayError);
  // EXPECT_TRUE(view.is_readonly());
  // EXPECT_TRUE(view.coords().is_readonly());
  // EXPECT_TRUE(view.masks().is_readonly());
  EXPECT_FALSE(view.data().is_readonly());
  EXPECT_FALSE(view.coords()[Dim::X].is_readonly());
  const auto copied(view);
  // For data arrays a shallow copy clears readonly flags, but this does not
  // appear useful for bins_view, since it just references the same buffer
  // (which is not shallow-copied).
  // EXPECT_TRUE(view.is_readonly());
  // EXPECT_TRUE(view.coords().is_readonly());
  // EXPECT_TRUE(view.masks().is_readonly());
  EXPECT_FALSE(copied.data().is_readonly());
  EXPECT_FALSE(copied.coords()[Dim::X].is_readonly());
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
