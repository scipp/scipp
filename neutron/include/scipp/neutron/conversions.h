// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <cmath>

namespace scipp::neutron::conversions {

constexpr auto tof_to_energy = [](auto &coord, const auto &c) {
  coord = c / (coord * coord);
};
constexpr auto energy_to_tof = [](auto &coord, const auto &c) {
  using std::sqrt;
  coord = sqrt(c / coord);
};
constexpr auto wavelength_to_q = [](auto &coord, const auto &c) {
  coord = c / coord;
};

} // namespace scipp::neutron::conversions
