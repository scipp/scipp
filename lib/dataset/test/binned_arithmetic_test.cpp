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
  Variable var = makeVariable<double>(Dims{Dim::Event}, Shape{5}, sc_units::m,
                                      Values{1, 2, 3, 4, 5});
  Variable expected = makeVariable<double>(Dims{Dim::Event}, Shape{5},
                                           sc_units::m, Values{1, 2, 6, 8, 10});
  DataArray array = DataArray(copy(var), {{Dim::X, var + var}});
};

TEST_F(BinnedArithmeticTest, fail_modify_slice_inplace_var) {
  Variable binned = make_bins(indices, Dim::Event, var);
  EXPECT_THROW(binned.slice({Dim::X, 1}) *= 2.0 * sc_units::m,
               except::UnitError);
  // unchanged
  EXPECT_EQ(binned, make_bins(indices, Dim::Event, var));
}

TEST_F(BinnedArithmeticTest, fail_modify_slice_inplace_array) {
  Variable binned = make_bins(indices, Dim::Event, array);
  EXPECT_THROW(binned.slice({Dim::X, 1}) *= 2.0 * sc_units::m,
               except::UnitError);
  // unchanged
  EXPECT_EQ(binned, make_bins(indices, Dim::Event, array));
}

TEST_F(BinnedArithmeticTest, modify_slice_inplace_var) {
  Variable binned = make_bins(indices, Dim::Event, var);
  binned.slice({Dim::X, 1}) *= 2.0 * sc_units::one;
  EXPECT_EQ(binned, make_bins(indices, Dim::Event, expected));
}

TEST_F(BinnedArithmeticTest, modify_slice_inplace_array) {
  Variable binned = make_bins(indices, Dim::Event, array);
  binned.slice({Dim::X, 1}) *= 2.0 * sc_units::one;
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
  // support coord and mask handling for binned data in such binary operations.
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

TEST_F(BinnedArithmeticTest, many_dims) {
  const auto indices6d = makeVariable<scipp::index_pair>(
      Dims{Dim{"a"}, Dim{"b"}, Dim{"c"}, Dim{"d"}, Dim{"e"}, Dim{"f"}},
      Shape{1, 1, 1, 1, 1, 1}, Values{std::pair{0, 1}});
  const auto buffer = makeVariable<int>(Dims{Dim::Event}, Shape{1}, Values{1});
  const auto binned = make_bins(indices6d, Dim::Event, buffer);
  const auto rhs = makeVariable<int>(Dims{}, Values{3});
  const auto expected_buffer =
      makeVariable<int>(Dims{Dim::Event}, Shape{1}, Values{4});
  EXPECT_EQ(binned + rhs, make_bins(indices6d, Dim::Event, expected_buffer));
}

TEST_F(BinnedArithmeticTest, too_many_dims) {
  // 6 outer dims + 1 event dim are 1 too many.
  const auto indices6d = makeVariable<scipp::index_pair>(
      Dims{Dim{"a"}, Dim{"b"}, Dim{"c"}, Dim{"d"}, Dim{"e"}, Dim{"f"}},
      Shape{2, 2, 2, 2, 2, 2},
      Values{std::pair{0, 1},   std::pair{1, 2},   std::pair{2, 3},
             std::pair{3, 4},   std::pair{4, 5},   std::pair{5, 6},
             std::pair{6, 7},   std::pair{7, 8},   std::pair{8, 9},
             std::pair{9, 10},  std::pair{10, 11}, std::pair{11, 12},
             std::pair{12, 13}, std::pair{13, 14}, std::pair{14, 15},
             std::pair{15, 16}, std::pair{16, 17}, std::pair{17, 18},
             std::pair{18, 19}, std::pair{19, 20}, std::pair{20, 21},
             std::pair{21, 22}, std::pair{22, 23}, std::pair{23, 24},
             std::pair{24, 25}, std::pair{25, 26}, std::pair{26, 27},
             std::pair{27, 28}, std::pair{28, 29}, std::pair{29, 30},
             std::pair{30, 31}, std::pair{31, 32}, std::pair{32, 33},
             std::pair{33, 34}, std::pair{34, 35}, std::pair{35, 36},
             std::pair{36, 37}, std::pair{37, 38}, std::pair{38, 39},
             std::pair{39, 40}, std::pair{40, 41}, std::pair{41, 42},
             std::pair{42, 43}, std::pair{43, 44}, std::pair{44, 45},
             std::pair{45, 46}, std::pair{46, 47}, std::pair{47, 48},
             std::pair{48, 49}, std::pair{49, 50}, std::pair{50, 51},
             std::pair{51, 52}, std::pair{52, 53}, std::pair{53, 54},
             std::pair{54, 55}, std::pair{55, 56}, std::pair{56, 57},
             std::pair{57, 58}, std::pair{58, 59}, std::pair{59, 60},
             std::pair{60, 61}, std::pair{61, 62}, std::pair{62, 63},
             std::pair{63, 64}});
  const auto buffer = makeVariable<int>(Dims{Dim::Event}, Shape{64});
  const auto binned = make_bins(indices6d, Dim::Event, buffer);
  // The values are still contiguous in `transposed` but with a non-contiguous
  // view. Making a copy then results in an operations where we iterate in a
  // non-contiguous manner which is not supported for this many dims.
  const auto transposed = binned.transpose(
      std::vector{Dim{"f"}, Dim{"e"}, Dim{"d"}, Dim{"c"}, Dim{"b"}, Dim{"a"}});
  EXPECT_THROW_DISCARD(copy(transposed), std::runtime_error);
}
