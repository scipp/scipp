// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <random>
#include <vector>

#include "scipp/variable/variable.h"

class Random {
  std::mt19937 mt{std::random_device()()};
  std::uniform_real_distribution<double> dist;

public:
  Random(double min = -2.0, double max = 2.0) : dist{min, max} {}
  std::vector<double> operator()(const int64_t size) {
    std::vector<double> data(size);
    std::generate(data.begin(), data.end(), [this]() { return dist(mt); });
    return data;
  }
  void seed(const uint32_t value) { mt.seed(value); }
};

class RandomBool {
  std::mt19937 mt{std::random_device()()};
  std::uniform_int_distribution<int32_t> dist;

public:
  RandomBool() : dist{0, 1} {}
  std::vector<bool> operator()(const int64_t size) {
    std::vector<bool> data(size);
    std::generate(data.begin(), data.end(), [this]() { return dist(mt); });
    return data;
  }
  void seed(const uint32_t value) { mt.seed(value); }
};

inline scipp::Variable makeRandom(const scipp::Dimensions &dims,
                                  const double min = -2.0,
                                  const double max = 2.0) {
  using namespace scipp;
  Random rand(min, max);
  return makeVariable<double>(Dimensions{dims}, Values(rand(dims.volume())));
}
