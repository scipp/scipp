// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Mads Bertelsen
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/universal_constants.hpp>

#include "scipp/core/counts.h"
#include "scipp/core/dataset.h"
#include "scipp/core/transform.h"
#include "scipp/core/except.h"
#include "scipp/neutron/beamline.h"
#include "scipp/neutron/convert.h"
#include "scipp/neutron/diffraction/convert_with_calibration.h"

using namespace scipp::core;

namespace scipp::neutron {

//using namespace scipp::core::except::expect;

namespace diffraction {

Dataset convert_with_calibration(Dataset d, const Dataset &cal){
  // 1. check cal has the required variables
  //scipp::core::expect::contains(cal, "tzero");
  //scipp::core::expect::contains(cal, "difc");

  // 2. Record ToF bin widths
  const auto oldBinWidths = counts::getBinWidths(d.coords(), {Dim::Tof});

  // 3. Transform coordinate
  d.setCoord(Dim::Tof, (d.coords()[Dim::Tof] - cal["tzero"].data()) / cal["difc"].data());

  // 4. Record DSpacing bin widths
  const auto newBinWidths = counts::getBinWidths(d.coords(), {Dim::Tof});

  // 5. Transform variables
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
} // namespace scipp::neutron
