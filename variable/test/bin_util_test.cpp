// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/bin_util.h"

using namespace scipp;
using namespace scipp::variable;

TEST(BinUtilTest, xxx) {
  const auto start =
      makeVariable<scipp::index>(Dims{Dim::X}, Shape{5}, Values{0, 0, 2, 2, 3});
  const auto stop =
      makeVariable<scipp::index>(Dims{Dim::X}, Shape{5}, Values{1, 3, 3, 4, 4});
  const auto subbin_sizes = makeVariable<scipp::index>(
      Dims{Dim("subbin")}, Shape{16},
      Values{1, 1, 1, 0, 0, 1, 1, 0, 2, 2, 0, 1, 1, 0, 0, 1});
  const scipp::index nbin = 2;
  const auto nsrc = start.dims().volume();
  const auto ndst = 4;
  const auto [subbin_offsets, output_bin_sizes] =
      variable::subbin_offsets(start, stop, subbin_sizes, nsrc, ndst, nbin);

  EXPECT_EQ(subbin_offsets,
            makeVariable<scipp::index>(
                subbin_sizes.dims(),
                Values{1, 3, 2, 3, 3, 4, 5, 7, 7, 9, 7, 10, 11, 11, 11, 12}));
  EXPECT_EQ(output_bin_sizes,
            makeVariable<scipp::index>(Dims{Dim("dst")}, Shape{ndst},
                                       Values{3, 1, 6, 2}));
}

TEST(SubbinSizesTest, contruct_fails) {
  EXPECT_THROW(SubbinSizes(1, 3, {}), except::SizeError);
  EXPECT_THROW(SubbinSizes(1, 3, {1}), except::SizeError);
  EXPECT_THROW(SubbinSizes(1, 3, {1, 2, 3}), except::SizeError);
  EXPECT_THROW(SubbinSizes(1, 0, {}), except::SizeError);
}

TEST(SubbinSizesTest, plus) {
  SubbinSizes a(1, 3, {2, 3});
  SubbinSizes b(1, 3, {3, 4});
  SubbinSizes c(0, 3, {1, 2, 3});
  SubbinSizes d(4, 5, {42});
  EXPECT_EQ(a + b, SubbinSizes(1, 3, {5, 7}));
  EXPECT_EQ(a + c, SubbinSizes(0, 3, {1, 4, 6}));
  EXPECT_EQ(a + d, SubbinSizes(1, 5, {2, 3, 0, 42}));
}
