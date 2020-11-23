// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/cumulative.h"

using namespace scipp;

TEST(CumulativeTest, exclusive_scan) {
  const auto var = makeVariable<int64_t>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                         Values{1, 2, 3, 4, 5, 6});
  EXPECT_EQ(exclusive_scan(var, Dim::X),
            makeVariable<int64_t>(var.dims(), Values{0, 0, 0, 1, 2, 3}));
  EXPECT_EQ(exclusive_scan(var, Dim::Y),
            makeVariable<int64_t>(var.dims(), Values{0, 1, 3, 0, 4, 9}));
}

TEST(CumulativeTest, exclusive_scan_bins) {
  const auto indices =
      makeVariable<scipp::index_pair>(Values{scipp::index_pair{0, 3}});
  const auto buffer =
      makeVariable<int64_t>(Dims{Dim::Row}, Shape{3}, Values{1, 2, 3});
  const auto var = make_bins(indices, Dim::Row, buffer);
  EXPECT_EQ(exclusive_scan_bins(var),
            make_bins(indices, Dim::Row,
                      makeVariable<int64_t>(buffer.dims(), Values{0, 1, 3})));
}
