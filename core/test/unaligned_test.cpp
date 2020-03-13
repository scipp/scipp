// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"
#include "scipp/core/groupby.h"
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
              << s.dim().name() << s.begin() << s.end();
        else
          EXPECT_EQ(slice.unaligned(), realigned.unaligned())
              << s.dim().name() << s.begin() << s.end();
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

TEST_F(RealignTest, slice_unaligned_view) {
  const auto realigned = make_realigned();
  const auto a = make_array();

  Slice s(Dim::Temperature, 0);
  EXPECT_EQ(realigned.unaligned().slice(s), a.slice(s));
}

auto extract_group(const GroupBy<DataArray> &grouped,
                   const scipp::index group) {
  const auto &slices = grouped.groups()[group];
  scipp::index size = 0;
  const auto &array = grouped.data();
  for (const auto &slice : slices)
    size += slice.end() - slice.begin();
  const Dim dim = array.coords()[grouped.dim()].dims().inner();
  auto out = copy(array.slice({dim, 0, size}));
  // TODO masks
  scipp::index current = 0;
  for (const auto &slice : slices) {
    const auto thickness = slice.end() - slice.begin();
    const Slice out_slice(slice.dim(), current, current + thickness);
    out.data().slice(out_slice).assign(array.data().slice(slice));
    for (const auto &[d, coord] : out.coords())
      if (coord.dims().contains(dim))
        coord.slice(out_slice).assign(array.coords()[d].slice(slice));
    current += thickness;
  }
  out.coords().erase(grouped.dim());
  return out;
}

TEST_F(RealignTest, histogram) {
  const auto realigned = make_realigned();
  // TODO Problem: This contains everything, but below we do not slice removed
  // dims (range slices are ok). Should we simply prevent non-range slicing?
  const auto &unaligned = realigned.unaligned();

  Variable data(unaligned.data(), realigned.dims());

  auto xgroups = groupby(unaligned, Dim::X, realigned.coords()[Dim::X]);
  for (scipp::index x = 0; x < realigned.dims()[Dim::X]; ++x) {
    auto xslice = extract_group(xgroups, x);

    auto ygroups = groupby(xslice, Dim::Y, realigned.coords()[Dim::Y]);
    for (scipp::index y = 0; y < realigned.dims()[Dim::Y]; ++y) {
      auto xyslice = extract_group(ygroups, y);

      auto hist1d = groupby(xyslice, Dim::Z, realigned.coords()[Dim::Z])
                        .sum(Dim::Position);
      data.slice({Dim::X, x}).slice({Dim::Y, y}).assign(hist1d.data());
    }
  }
  EXPECT_EQ(data, make_aligned().data());
}
