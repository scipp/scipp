// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"
#include "scipp/core/unaligned.h"

using namespace scipp;
using namespace scipp::core;

TEST(UnalignedTest, align) {
  const Dim dim = Dim("position");
  const auto pos = makeVariable<Eigen::Vector3d>(
      Dims{dim}, Shape{4},
      Values{Eigen::Vector3d{1, 1, 1}, Eigen::Vector3d{1, 1, 2},
             Eigen::Vector3d{1, 2, 3}, Eigen::Vector3d{1, 2, 4}});
  const auto x = makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 1, 1, 1});
  const auto y = makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 1, 2, 2});
  const auto z = makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 2, 3, 4});
  DataArray base(makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 2, 3, 4}),
                 {{dim, pos}, {Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}});

  base = concatenate(base, base + base, Dim::Temperature);
  EXPECT_EQ(base.dims(), Dimensions({Dim::Temperature, Dim::Position}, {2, 4}));
  base.coords().set(Dim::Temperature,
                    makeVariable<double>(Dims{Dim::Temperature}, Shape{2}));

  const auto xbins = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 4});
  const auto ybins = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0, 4});
  const auto zbins = makeVariable<double>(Dims{Dim::Z}, Shape{2}, Values{0, 4});

  auto aligned = unaligned::align(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_FALSE(aligned.hasData());
  EXPECT_EQ(
      aligned.dims(),
      Dimensions({Dim::Temperature, Dim::Z, Dim::Y, Dim::X}, {2, 1, 1, 1}));
  // EXPECT_TRUE(aligned.coords().contains(Dim::X));
  // EXPECT_TRUE(aligned.coords().contains(Dim::Y));
  // EXPECT_TRUE(aligned.coords().contains(Dim::Z));
  // EXPECT_EQ(aligned.coords()[Dim::X], xbins);
  // EXPECT_EQ(aligned.coords()[Dim::Y], ybins);
  // EXPECT_EQ(aligned.coords()[Dim::Z], zbins);
}

