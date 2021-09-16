// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

  /**
   * Generate a random k-subset of the integer interval [0, n].
   * Uses a very inefficient implementation only suitable for small n.
   */
  std::vector<scipp::index> random_subset_of_interval(const scipp::index n,
                                                      const scipp::index k) {
    if (k == 0) {
      return {};
    }
    if (k > n) {
      throw std::invalid_argument("k > n not allowed");
    }
    if (n == 0) {
      throw std::invalid_argument("n must not be 0");
    }

    std::vector<scipp::index> available(static_cast<size_t>(n));
    std::iota(available.begin(), available.end(), 0);

    std::vector<scipp::index> selected(static_cast<size_t>(k));
    std::generate(
        selected.begin(), selected.end(), [&available, this]() mutable {
          std::uniform_int_distribution<size_t> int_dist(1,
                                                         available.size() - 1);
          const auto index = int_dist(this->mt);
          const auto val = available.at(index);
          available.erase(available.cbegin() + static_cast<ssize_t>(index));
          return val;
        });
    return selected;
  }

  /**
   * Generate a random partition of the integer n into k parts.
   *
   * Implements algorithm 'RANCOM' from
   * 'Combinatorial Algorithms for Computers and Calculators'
   * by A. Nijenhuis, H. S. Wilf.
   *
   * See also index_pairs_from_sizes.
   */
  std::vector<scipp::index> random_int_partition(const scipp::index n,
                                                 const scipp::index k) {
    if (k == 0) {
      return {};
    }
    if (k > n) {
      throw std::invalid_argument("k > n not allowed");
    }
    if (n == 0) {
      throw std::invalid_argument("n must not be 0");
    }

    auto a = random_subset_of_interval(n + k - 1, k - 1);
    std::sort(a.begin(), a.end());
    std::vector<scipp::index> r(static_cast<size_t>(k));
    std::transform(a.begin(), a.end(), r.begin(),
                   [previous = 0](const auto aj) mutable {
                     auto res = aj - std::exchange(previous, aj) - 1;
                     return res;
                   });
    r.back() = n + k - 1 - (!a.empty() ? a.back() : 0);
    return r;
  }
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
