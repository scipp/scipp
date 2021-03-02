// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <random>
#include <vector>

#include "scipp/common/traits.h"
#include "scipp/variable/variable.h"

namespace scipp::testing {
namespace detail {
template <class T> struct identity {
  constexpr auto operator()(const T &x) const { return x; }
};
} // namespace detail

class Random {
  std::mt19937 m_mt{std::random_device()()};
  double m_min;
  double m_max;

public:
  explicit Random(double min = -10.0, double max = 10.0)
      : m_min{min}, m_max{max} {}

  void seed(const uint32_t value) { m_mt.seed(value); }

  template <class T, class F = detail::identity<T>>
  auto generate(const scipp::index size,
                const F &&transform = detail::identity<T>{}) {
    std::vector<T> data(size);
    if constexpr (std::is_floating_point_v<T>) {
      std::uniform_real_distribution<T> dist{m_min, m_max};
      std::generate(
          data.begin(), data.end(),
          [this, dist, &transform]() mutable { return transform(dist(m_mt)); });
    } else if constexpr (std::is_integral_v<T>) {
      std::uniform_int_distribution<T> dist{static_cast<T>(m_min),
                                            static_cast<T>(m_max)};
      std::generate(
          data.begin(), data.end(),
          [this, dist, &transform]() mutable { return transform(dist(m_mt)); });
    } else if constexpr (std::is_same_v<T, scipp::core::time_point>) {
      std::uniform_int_distribution<int64_t> dist{static_cast<int64_t>(m_min),
                                                  static_cast<int64_t>(m_max)};
      std::generate(data.begin(), data.end(),
                    [this, dist, &transform]() mutable {
                      return transform(scipp::core::time_point{dist(m_mt)});
                    });
    } else {
      static_assert(
          scipp::common::always_false<T>,
          "Random data generation is not implemented for the given type.");
    }
    return data;
  }

  std::vector<double> operator()(const int64_t size) {
    return generate<double>(size);
  }

  [[nodiscard]] scipp::variable::Variable
  make_variable(const scipp::Dimensions &dims, scipp::core::DType dtype,
                scipp::units::Unit unit = scipp::units::one,
                bool with_variances = false);
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

scipp::Variable makeRandom(const scipp::Dimensions &dims, double min = -2.0,
                           double max = 2.0);
} // namespace scipp::testing