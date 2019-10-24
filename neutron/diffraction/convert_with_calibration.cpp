// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Mads Bertelsen
#include "scipp/core/counts.h"
#include "scipp/neutron/diffraction/convert_with_calibration.h"

using namespace scipp::core;

namespace scipp::neutron::diffraction {

Dataset convert_with_calibration(Dataset d, const Dataset &cal){
  // 1. Record ToF bin widths
  const auto oldBinWidths = counts::getBinWidths(d.coords(), {Dim::Tof});

  // 2. Transform coordinate
  d.setCoord(Dim::Tof, (d.coords()[Dim::Tof] - cal["tzero"].data()) / cal["difc"].data());

  // 3. Record DSpacing bin widths
  const auto newBinWidths = counts::getBinWidths(d.coords(), {Dim::Tof});

  // 4. Transform variables
  for (const auto & [ name, data ] : d) {
    static_cast<void>(name);
    if (data.coords()[Dim::Tof].dims().sparse()) {
      data.coords()[Dim::Tof].assign(
          data.coords()[Dim::Tof] - cal["tzero"].data());
      data.coords()[Dim::Tof].assign(
          data.coords()[Dim::Tof] / cal["difc"].data());
    } else if (data.unit().isCountDensity()) {
      counts::fromDensity(data, oldBinWidths);
      counts::toDensity(data, newBinWidths);
    }
  }

  d.rename(Dim::Tof, Dim::DSpacing);
  return d;
}

} // namespace scipp::neutron::diffraction
