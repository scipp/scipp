// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"
#include "scipp/core/histogram.h"
#include "scipp/core/unaligned.h"

using namespace scipp;
using namespace scipp::core;

struct RealignTest : public ::testing::Test {
protected:
  Variable temp = makeVariable<double>(Dims{Dim::Temperature}, Shape{2});
  Variable xbins =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 2, 4});
  Variable ybins =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{0, 2, 4});
  Variable zbins =
      makeVariable<double>(Dims{Dim::Z}, Shape{3}, Values{0, 2, 4});

  DataArray make_array() {
    const Dim dim = Dim::Position;
    const auto pos = makeVariable<Eigen::Vector3d>(
        Dims{dim}, Shape{4},
        Values{Eigen::Vector3d{1, 1, 1}, Eigen::Vector3d{1, 1, 2},
               Eigen::Vector3d{1, 2, 3}, Eigen::Vector3d{1, 2, 4}});
    const auto x =
        makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 1, 1, 1});
    const auto y =
        makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 1, 2, 2});
    const auto z =
        makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 2, 3, 4});
    DataArray a(makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 2, 3, 4}),
                {{dim, pos}, {Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}});

    a = concatenate(a, a + a, Dim::Temperature);
    EXPECT_EQ(a.dims(), Dimensions({Dim::Temperature, Dim::Position}, {2, 4}));
    a.coords().set(Dim::Temperature, temp);
    return a;
  }

  DataArray make_realigned() {
    return unaligned::realign(
        make_array(), {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});
  }

  DataArray make_aligned() {
    return DataArray(
        makeVariable<double>(
            Dims{Dim::Temperature, Dim::Z, Dim::Y, Dim::X}, Shape{2, 2, 2, 2},
            Values{1, 0, 0, 0, 2, 0, 3, 0, 2, 0, 0, 0, 4, 0, 6, 0}),
        {{Dim::Temperature, temp},
         {Dim::Z, zbins},
         {Dim::Y, ybins},
         {Dim::X, xbins}});
  }
};

TEST_F(RealignTest, basics) {
  const auto reference = make_aligned();
  auto base = make_array();
  auto realigned = unaligned::realign(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_FALSE(realigned.hasData());
  EXPECT_EQ(realigned.dims(), reference.dims());
  EXPECT_EQ(realigned.coords(), reference.coords());

  EXPECT_EQ(realigned.unaligned(), base);
}

TEST_F(RealignTest, dimension_order) {
  auto base = make_array();
  DataArray transposed(Variable(base.data().transpose()), base.coords());
  auto realigned1 = unaligned::realign(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});
  auto realigned2 = unaligned::realign(
      transposed, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_FALSE(realigned1.hasData());
  EXPECT_FALSE(realigned2.hasData());
  EXPECT_EQ(
      realigned1.dims(),
      Dimensions({Dim::Temperature, Dim::Z, Dim::Y, Dim::X}, {2, 2, 2, 2}));
  // Dim::Position is outside Dim::Temperature, when mapping position to X, Y,
  // and Z stays the inner dim.
  EXPECT_EQ(
      realigned2.dims(),
      Dimensions({Dim::Z, Dim::Y, Dim::X, Dim::Temperature}, {2, 2, 2, 2}));
}

TEST_F(RealignTest, mask_mapping) {
  auto base = make_array();
  base.masks().set("pos",
                   makeVariable<bool>(Dims{Dim::Position}, Shape{4},
                                      Values{false, false, false, true}));
  base.masks().set("temp", makeVariable<bool>(Dims{Dim::Temperature}, Shape{2},
                                              Values{false, true}));
  auto realigned = unaligned::realign(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_EQ(realigned.masks().size(), 1);
  EXPECT_TRUE(realigned.masks().contains("temp"));

  EXPECT_EQ(realigned.unaligned(), base);
}

TEST_F(RealignTest, attr_mapping) {
  auto base = make_array();
  base.attrs().set("0-d", makeVariable<double>(Values{1.0}));
  base.attrs().set("pos",
                   makeVariable<bool>(Dims{Dim::Position}, Shape{4},
                                      Values{false, false, false, true}));
  base.attrs().set("temp", makeVariable<bool>(Dims{Dim::Temperature}, Shape{2},
                                              Values{false, true}));
  auto realigned = unaligned::realign(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_FALSE(realigned.hasData());
  EXPECT_EQ(realigned.attrs().size(), 2);
  EXPECT_TRUE(realigned.attrs().contains("0-d"));
  EXPECT_TRUE(realigned.attrs().contains("temp"));

  EXPECT_EQ(realigned.unaligned(), base);
}

TEST_F(RealignTest, realigned_bounds) {
  const auto realigned = make_realigned();
  DataArrayConstView view(realigned);

  auto bounds = view.slice_bounds();
  EXPECT_EQ(bounds.size(), 0);

  view = view.slice({Dim::X, 1, 2});
  bounds = view.slice_bounds();
  EXPECT_EQ(bounds.size(), 1);
  EXPECT_EQ(bounds.at(0).first, Dim::X);
  EXPECT_EQ(bounds.at(0).second,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4}));

  view = view.slice({Dim::Y, 0, 2});
  bounds = view.slice_bounds();
  EXPECT_EQ(bounds.size(), 2);
  EXPECT_EQ(bounds.at(0).first, Dim::X);
  EXPECT_EQ(bounds.at(0).second,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4}));
  EXPECT_EQ(bounds.at(1).first, Dim::Y);
  EXPECT_EQ(bounds.at(1).second,
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0, 4}));

  // Slice again in same dimension
  view = view.slice({Dim::X, 0});
  bounds = view.slice_bounds();
  EXPECT_EQ(bounds.size(), 2);
  EXPECT_EQ(bounds.at(0).first, Dim::X);
  EXPECT_EQ(bounds.at(0).second,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4}));
  EXPECT_EQ(bounds.at(1).first, Dim::Y);
  EXPECT_EQ(bounds.at(1).second,
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0, 4}));
}

TEST_F(RealignTest, copy_realigned) {
  const auto realigned = make_realigned();
  EXPECT_EQ(DataArray(realigned), realigned);
  EXPECT_EQ(DataArray(DataArrayConstView(realigned)), realigned);
}

TEST_F(RealignTest, copy_realigned_slice) {
  const auto realigned = make_realigned();
  const auto slice = realigned.slice({Dim::Z, 1});
  // `slice` contains unfiltered unaligned content, but copy drops out-of-bounds
  // content.
  EXPECT_NE(DataArray(slice), slice);
}

TEST_F(RealignTest, slice) {
  const auto realigned = make_realigned();
  const auto aligned = make_aligned();

    for (const auto dim : {Dim::Temperature, Dim::X, Dim::Y, Dim::Z}) {
      for (const auto s : {Slice(dim, 0), Slice(dim, 1), Slice(dim, 0, 1),
                           Slice(dim, 0, 2), Slice(dim, 1, 2)}) {
        const auto slice = realigned.slice(s);
        const auto reference = aligned.slice(s);
        // Same result as when slicing normal array, except for missing data
        EXPECT_FALSE(slice.hasData());
        EXPECT_EQ(slice.dims(), reference.dims());
        EXPECT_EQ(slice.coords(), reference.coords());
        if (dim == Dim::Temperature)
          EXPECT_EQ(slice.unaligned(), realigned.unaligned().slice(s))
              << to_string(s);
        else
          EXPECT_EQ(slice.unaligned(), realigned.unaligned()) << to_string(s);
    }
  }
}

TEST_F(RealignTest, unaligned_of_slice_along_aligned_dim) {
  const auto realigned = make_realigned();
  const auto unaligned = make_array();

  // Dim::Temperature is a dim of both the wrapper and the unaligned content.
  Slice s(Dim::Temperature, 0);
  EXPECT_EQ(realigned.slice(s).unaligned(), unaligned.slice(s));
}

TEST_F(RealignTest, unaligned_of_slice_along_realigned_dim) {
  const auto realigned = make_realigned();
  const auto unaligned = make_array();

  // Dim::X is a dim of the wrapper but not the unaligned content. For now
  // slicing the wrapper returns a view on the full unaligned content, *not*
  // filtering any "events".
  Slice s(Dim::X, 0);
  EXPECT_EQ(realigned.slice(s).unaligned(), unaligned);
}

TEST_F(RealignTest, unaligned_slice_contains_sliced_coords) {
  // This is implied by test `unaligned_of_slice_along_realigned_dim` but
  // demonstrates more explicitly how coordinates (and dimensions) are
  // preserved.
  const auto realigned = make_realigned();
  const auto slice = realigned.slice({Dim::X, 0});
  EXPECT_FALSE(slice.coords().contains(Dim::X));
  // Slicing realigned dimensions does not eagerly slice the unaligned content.
  // Therefore, corresponding coordinates are not removed, even for a non-range
  // slice.
  EXPECT_TRUE(slice.unaligned().coords().contains(Dim::X));
}

TEST_F(RealignTest, slice_unaligned_view) {
  const auto realigned = make_realigned();
  const auto a = make_array();

  Slice s(Dim::Temperature, 0);
  EXPECT_EQ(realigned.unaligned().slice(s), a.slice(s));
}

TEST_F(RealignTest, histogram) {
  const auto realigned = make_realigned();
  EXPECT_EQ(histogram(realigned), make_aligned());
}

TEST_F(RealignTest, histogram_transposed) {
  auto base = make_array();
  DataArray transposed(Variable(base.data().transpose()), base.coords());
  auto realigned = unaligned::realign(
      transposed, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});
  EXPECT_NO_THROW(histogram(realigned));
}

TEST_F(RealignTest, histogram_slice) {
  const auto realigned = make_realigned();
  const auto aligned = make_aligned();
  for (const auto dim : {Dim::Temperature, Dim::X, Dim::Y, Dim::Z}) {
    for (const auto s : {Slice(dim, 0), Slice(dim, 1), Slice(dim, 0, 1),
                         Slice(dim, 0, 2), Slice(dim, 1, 2)}) {
      const auto slice = realigned.slice(s);
      EXPECT_EQ(histogram(slice), aligned.slice(s)) << to_string(s);
    }
  }
}
