// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/shape.h"

using namespace scipp;

TEST(CumulativeTest, cumsum) {
  const auto var = makeVariable<int64_t>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                         Values{1, 2, 3, 4, 5, 6});
  // inclusive (default)
  EXPECT_EQ(cumsum(var, Dim::X),
            makeVariable<int64_t>(var.dims(), Values{1, 2, 3, 5, 7, 9}));
  EXPECT_EQ(cumsum(var, Dim::Y),
            makeVariable<int64_t>(var.dims(), Values{1, 3, 6, 4, 9, 15}));
  EXPECT_EQ(cumsum(var),
            makeVariable<int64_t>(var.dims(), Values{1, 3, 6, 10, 15, 21}));
  // exclusive
  EXPECT_EQ(cumsum(var, Dim::X, CumSumMode::Exclusive),
            makeVariable<int64_t>(var.dims(), Values{0, 0, 0, 1, 2, 3}));
  EXPECT_EQ(cumsum(var, Dim::Y, CumSumMode::Exclusive),
            makeVariable<int64_t>(var.dims(), Values{0, 1, 3, 0, 4, 9}));
  EXPECT_EQ(cumsum(var, CumSumMode::Exclusive),
            makeVariable<int64_t>(var.dims(), Values{0, 1, 3, 6, 10, 15}));
}

TEST(CumulativeTest, cumsum_bins) {
  const auto indices =
      makeVariable<scipp::index_pair>(Values{scipp::index_pair{0, 3}});
  const auto buffer =
      makeVariable<int64_t>(Dims{Dim::Row}, Shape{3}, Values{1, 2, 3});
  const auto var = make_bins(indices, Dim::Row, buffer);
  EXPECT_EQ(cumsum_bins(var),
            make_bins(indices, Dim::Row,
                      makeVariable<int64_t>(buffer.dims(), Values{1, 3, 6})));
  EXPECT_EQ(cumsum_bins(var, CumSumMode::Exclusive),
            make_bins(indices, Dim::Row,
                      makeVariable<int64_t>(buffer.dims(), Values{0, 1, 3})));
}

class CumulativePrecisionTest : public ::testing::Test {
protected:
  const float init = 100000000.0;
  Variable var = makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                     Values{init, 1.f, 1.f, 1.f, 1.f, 1.f});
  Variable expected =
      makeVariable<float>(var.dims(), Values{init + 0, init + 1, init + 2,
                                             init + 3, init + 4, init + 5});
};

TEST_F(CumulativePrecisionTest, cumsum) { EXPECT_EQ(cumsum(var), expected); }

TEST_F(CumulativePrecisionTest, cumsum_bins) {
  const auto indices =
      makeVariable<scipp::index_pair>(Values{scipp::index_pair{0, 6}});
  const auto buffer = flatten(var, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Row);
  var = make_bins(indices, Dim::Row, buffer);
  expected = flatten(expected, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Row);
  EXPECT_EQ(cumsum_bins(var), make_bins(indices, Dim::Row, expected));
}
