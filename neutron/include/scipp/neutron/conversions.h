// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <cmath>
#include <limits>

#include "scipp/units/unit.h"

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
  if constexpr (!std::is_same_v<std::decay_t<decltype(coord)>, units::Unit>) {
    if (tof <= 0.0) {
      if (energy_shift >= 0.0)
        coord = std::numeric_limits<double>::max();
      else
        coord = -std::numeric_limits<double>::max();
      return;
    }
  }
  coord = scale / (tof * tof) - energy_shift;
};
constexpr auto energy_transfer_to_tof = [](auto &coord, const auto &scale,
                                           const auto &tof_shift,
                                           const auto &energy_shift) {
  coord = tof_shift + sqrt(scale / (coord + energy_shift));
};

} // namespace scipp::neutron::conversions
