// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/string.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/shape.h"

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
  DataArray array = DataArray(copy(var), {{Dim::X, var + var}});
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
  DataArray expected_array = DataArray(expected, {{Dim::X, var + var}});
  EXPECT_EQ(binned, make_bins(indices, Dim::Event, expected_array));
}

TEST_F(BinnedArithmeticTest, var_and_var) {
  Variable binned = make_bins(indices, Dim::Event, var);
  EXPECT_EQ(binned + binned, make_bins(indices, Dim::Event, var + var));
}

TEST_F(BinnedArithmeticTest, fail_array_and_array) {
  const auto binned = make_bins(indices, Dim::Event, array);
  // In principle the operation could be allowed in this case since the coords
  // match, but at this point our implementation is not sophisticated enough to
  // support coord, mask, and attr handling for binned data in such binary
  // operations.
  EXPECT_THROW_DISCARD(binned + binned, except::BinnedDataError);
}

TEST_F(BinnedArithmeticTest, var_and_array) {
  const auto expected_array = DataArray(var + var, {{Dim::X, var + var}});
  Variable binned_var = make_bins(indices, Dim::Event, var);
  Variable binned_arr = make_bins(indices, Dim::Event, array);
  const auto var_arr = binned_var + binned_arr;
  const auto arr_var = binned_arr + binned_var;
  // Summing variables and data arrays must return data arrays
  EXPECT_EQ(var_arr.dtype(), binned_arr.dtype());
  EXPECT_EQ(arr_var.dtype(), binned_arr.dtype());
  EXPECT_EQ(var_arr, make_bins(indices, Dim::Event, expected_array));
  EXPECT_EQ(arr_var, make_bins(indices, Dim::Event, expected_array));
}

TEST_F(BinnedArithmeticTest, op_on_transpose) {
  const auto indices2d =
      makeVariable<scipp::index_pair>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                      Values{std::pair{0, 1}, std::pair{1, 1},
                                             std::pair{2, 4}, std::pair{4, 5}});
  const auto binned = make_bins(indices2d, Dim::Event, array);
  const auto transposed = transpose(binned);
  const auto one = makeVariable<double>(Values{1});
  EXPECT_EQ(transposed * one, copy(transposed));
}

TEST_F(BinnedArithmeticTest, op_on_transpose_with_dtype_change) {
  const auto indices2d =
      makeVariable<scipp::index_pair>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                      Values{std::pair{0, 1}, std::pair{1, 1},
                                             std::pair{2, 4}, std::pair{4, 5}});
  DataArray int_array =
      DataArray(astype(var, dtype<int>), {{Dim::X, var + var}});
  const auto binned = make_bins(indices2d, Dim::Event, int_array);
  const auto transposed = transpose(binned);
  const auto one = makeVariable<double>(Values{1});
  EXPECT_EQ(transposed * one, copy(transposed) * one);
}

TEST_F(BinnedArithmeticTest, op_on_slice) {
  const auto binned = make_bins(indices, Dim::Event, array);
  const auto slice = binned.slice({Dim::X, 1, 2});
  const auto one = makeVariable<double>(Values{1});
  EXPECT_EQ(slice * one, copy(slice));
}
