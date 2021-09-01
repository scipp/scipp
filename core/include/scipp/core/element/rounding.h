// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Samuel Jones
#pragma once

#include <cmath>

#include "scipp/common/index.h"

namespace scipp::core::element {

constexpr auto floor = [](const auto &a) { return std::floor(a); };

constexpr auto ceil = [](const auto &a) { return std::ceil(a); };

constexpr auto round = [](const auto &a, const scipp::index decimals = 0) {
  if (decimals != 0) {
    const auto multiplier = std::pow(10.0, decimals);
    return std::rint(a * multiplier) / multiplier;
  } else {
    return std::rint(a);
  }
};

} // namespace scipp::core::element