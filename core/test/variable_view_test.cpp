/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <numeric>

#include "variable_view.h"

using namespace scipp::core;

TEST(VariableView, full_volume) {
  Dimensions dims({{Dim::Y, 4}, {Dim::X, 2}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);
  VariableView<double> view(variable.data(), 0, dims, dims);
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
  Dimensions dims({{Dim::Y, 4}, {Dim::X, 2}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  Dimensions variableDims({{Dim::Y, 3}, {Dim::X, 1}});
  VariableView<double> view(variable.data(), 0, variableDims, dims);
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 3);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 4.0);
}

TEST(VariableView, edges_first) {
  Dimensions dims({{Dim::Y, 4}, {Dim::X, 2}});
  Dimensions edgeDims({{Dim::Y, 4}, {Dim::X, 3}});
  std::vector<double> variable(edgeDims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  VariableView<double> view(variable.data(), 0, dims, edgeDims);
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
  Dimensions dims({{Dim::Y, 4}, {Dim::X, 2}});
  Dimensions edgeDims({{Dim::Y, 5}, {Dim::X, 2}});
  std::vector<double> variable(edgeDims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  VariableView<double> view(variable.data(), 0, dims, edgeDims);
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
  Dimensions dims({{Dim::Y, 3}, {Dim::X, 2}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  Dimensions variableDims({{Dim::Y, 3}});
  VariableView<double> view(variable.data(), 0, variableDims, dims);
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 3);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 4.0);

  Dimensions subDims({{Dim::Y, 3}, {Dim::X, 2}});
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
