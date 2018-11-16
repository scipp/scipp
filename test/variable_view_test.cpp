/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <numeric>

#include "variable_view.h"

TEST(VariableView, full_volume) {
  Dimensions dims({{Dim::X, 2}, {Dim::Y, 4}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);
  VariableView<double> view(variable.data(), dims, dims);
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 8);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 1.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 3.0);
  EXPECT_EQ(*it++, 4.0);
  EXPECT_EQ(*it++, 5.0);
  EXPECT_EQ(*it++, 6.0);
  EXPECT_EQ(*it++, 7.0);
}

TEST(VariableView, subvolume) {
  Dimensions dims({{Dim::X, 2}, {Dim::Y, 4}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  Dimensions variableDims({{Dim::X, 1}, {Dim::Y, 3}});
  VariableView<double> view(variable.data(), variableDims, dims);
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 3);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 4.0);
}

TEST(VariableView, edges_first) {
  Dimensions dims({{Dim::X, 2}, {Dim::Y, 4}});
  Dimensions edgeDims({{Dim::X, 3}, {Dim::Y, 4}});
  std::vector<double> variable(edgeDims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  VariableView<double> view(variable.data(), dims, edgeDims);
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 8);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 1.0);
  EXPECT_EQ(*it++, 3.0);
  EXPECT_EQ(*it++, 4.0);
  EXPECT_EQ(*it++, 6.0);
  EXPECT_EQ(*it++, 7.0);
  EXPECT_EQ(*it++, 9.0);
  EXPECT_EQ(*it++, 10.0);
}

TEST(VariableView, edges_second) {
  Dimensions dims({{Dim::X, 2}, {Dim::Y, 4}});
  Dimensions edgeDims({{Dim::X, 2}, {Dim::Y, 5}});
  std::vector<double> variable(edgeDims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  VariableView<double> view(variable.data(), dims, edgeDims);
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 8);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 1.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 3.0);
  EXPECT_EQ(*it++, 4.0);
  EXPECT_EQ(*it++, 5.0);
  EXPECT_EQ(*it++, 6.0);
  EXPECT_EQ(*it++, 7.0);
}

TEST(VariableView, subview) {
  Dimensions dims({{Dim::X, 2}, {Dim::Y, 3}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  Dimensions variableDims({{Dim::Y, 3}});
  VariableView<double> view(variable.data(), variableDims, dims);
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 3);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 4.0);

  Dimensions subDims({{Dim::X, 2}, {Dim::Y, 3}});
  VariableView<double> subView(view, subDims);
  it = subView.begin();
  ASSERT_EQ(std::distance(it, subView.end()), 6);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 4.0);
  EXPECT_EQ(*it++, 4.0);
}
