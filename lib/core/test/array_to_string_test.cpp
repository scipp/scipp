// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gmock/gmock-matchers.h>
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <regex>
#include <vector>

#include "scipp/core/array_to_string.h"
#include "scipp/core/element_array.h"
#include "scipp/core/element_array_view.h"

using namespace scipp;
using namespace scipp::core;

using ::testing::ContainerEq;

namespace {
template <class T> auto array_to_string(const std::vector<T> &array) {
  const Dimensions dims{Dim::X, scipp::size(array)};
  const ElementArrayViewParams params{0, dims, Strides{dims}, BucketParams{}};
  const ElementArrayView view{params, array.data()};
  return array_to_string(view, std::nullopt);
}

constexpr auto FLOAT_REGEX = R"([-+]?(?:\d*\.)?\d+(?:e[-+]?\d+)?)";

auto match_numbers(const std::string &text) {
  const std::regex regex{FLOAT_REGEX};
  std::vector<double> numbers;
  std::transform(std::sregex_iterator(text.begin(), text.end(), regex),
                 std::sregex_iterator(), std::back_inserter(numbers),
                 [](auto &match) {
                   assert(match.size() == 1);
                   return std::stod(match.str(0));
                 });
  return numbers;
}
} // namespace

/*
 * The exact formatting (separators, where to put spaces, etc.) is not
 * important. The tests here only check if the correct numbers appear in the
 * output in the correct order. Since the functions use equality comparisons of
 * floating point numbers, the inputs must be suitably chosen such that no
 * digits are lost in the output.
 */

TEST(array_to_string, double) {
  const std::vector<double> array({1.0, -5.9, 1.3e-9, 2.1e11});
  const auto matched = match_numbers(array_to_string(array));
  EXPECT_THAT(matched, ContainerEq(array));
}

TEST(array_to_string, Vector3d) {
  const std::vector<double> buffer{1.0, 2.3, -4.5, -1e-11, 0.234, 2.1e8};
  std::vector<Eigen::Vector3d> array(2);
  for (std::size_t i = 0; i < array.size(); ++i) {
    array[i] = Eigen::Vector3d{buffer[i * 3 + 0], buffer[i * 3 + 1],
                               buffer[i * 3 + 2]};
  }
  const auto matched = match_numbers(array_to_string(array));
  EXPECT_THAT(matched, ContainerEq(buffer));
}

TEST(array_to_string, Matrix3d) {
  const std::vector<double> buffer{1.0,   2.3,     -4.5, 6.7,    -8.9,  0.12,
                                   -2.01, -3,      7.3,  -1e-11, 0.234, 2.1e8,
                                   1.3e7, -3.4e12, 0.32, -12,    4e-3,  5e-9};
  assert(buffer.size() == 9 + 9);
  std::vector<Eigen::Matrix3d> array(2);
  for (std::size_t i = 0; i < array.size(); ++i) {
    Eigen::Matrix3d mat;
    for (Eigen::Index row = 0; row < 3; ++row) {
      for (Eigen::Index col = 0; col < 3; ++col) {
        mat(row, col) = buffer[i * 9 + row * 3 + col];
      }
    }
    array[i] = mat;
  }
  const auto matched = match_numbers(array_to_string(array));
  EXPECT_THAT(matched, ContainerEq(buffer));
}

TEST(array_to_string, Quaternion) {
  const std::vector<double> buffer{1.0,  2.0,   -3.0,   4.0,
                                   -0.1, 1e-10, 2.3e13, -1.234};
  std::vector<Quaternion> array(2);
  for (std::size_t i = 0; i < array.size(); ++i) {
    array[i] = Quaternion{{buffer[i * 4 + 0], buffer[i * 4 + 1],
                           buffer[i * 4 + 2], buffer[i * 4 + 3]}};
  }
  const auto matched = match_numbers(array_to_string(array));
  EXPECT_THAT(matched, ContainerEq(buffer));
}
