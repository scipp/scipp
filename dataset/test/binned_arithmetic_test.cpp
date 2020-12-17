// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::dataset;

class BinnedArithmeticTest : public ::testing::Test {
protected:
  Variable indices = makeVariable<scipp::index_pair>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 5}});
  Variable var = makeVariable<double>(Dims{Dim::Event}, Shape{5}, units::m,
                                      Values{1, 2, 3, 4, 5});
  DataArray array = DataArray(var, {{Dim::X, var + var}});
};

TEST_F(BinnedArithmeticTest, slice_inplace) {
  Variable binned = make_bins(indices, Dim::Event, var);
  binned.slice({Dim::X, 1}) *= 2.0 * units::one;
}
