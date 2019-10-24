// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_TEST_RANDOM_H
#define SCIPP_TEST_RANDOM_H

#include <algorithm>
#include <random>
#include <vector>

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
};

#endif // SCIPP_TEST_RANDOM_H
