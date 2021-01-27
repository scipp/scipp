// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
constexpr auto tof_to_energy_transfer = [](auto &coord, const auto &scale,
                                           const auto &tof_shift,
                                           const auto &energy_shift) {
  const auto tof = (coord - tof_shift);
  coord = scale / (tof * tof) - energy_shift;
};

} // namespace scipp::neutron::conversions
