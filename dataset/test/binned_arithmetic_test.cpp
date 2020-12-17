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
  Variable expected = makeVariable<double>(Dims{Dim::Event}, Shape{5}, units::m,
                                           Values{1, 2, 6, 8, 10});
  DataArray array = DataArray(var, {{Dim::X, var + var}});
  DataArray expected_array = DataArray(expected, {{Dim::X, var + var}});
};

TEST_F(BinnedArithmeticTest, fail_modify_slice_inplace_var) {
  Variable binned = make_bins(indices, Dim::Event, var);
  EXPECT_THROW(binned.slice({Dim::X, 1}) *= 2.0 * units::m, except::UnitError);
  // unchanged
  EXPECT_EQ(binned, make_bins(indices, Dim::Event, var));
}

TEST_F(BinnedArithmeticTest, fail_modify_slice_inplace_array) {
  Variable binned = make_bins(indices, Dim::Event, array);
  EXPECT_THROW(binned.slice({Dim::X, 1}) *= 2.0 * units::m, except::UnitError);
  // unchanged
  EXPECT_EQ(binned, make_bins(indices, Dim::Event, array));
}

TEST_F(BinnedArithmeticTest, modify_slice_inplace_var) {
  Variable binned = make_bins(indices, Dim::Event, var);
  binned.slice({Dim::X, 1}) *= 2.0 * units::one;
  EXPECT_EQ(binned, make_bins(indices, Dim::Event, expected));
}

TEST_F(BinnedArithmeticTest, modify_slice_inplace_array) {
  Variable binned = make_bins(indices, Dim::Event, array);
  binned.slice({Dim::X, 1}) *= 2.0 * units::one;
  EXPECT_EQ(binned, make_bins(indices, Dim::Event, expected_array));
}
