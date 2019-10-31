// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Mads Bertelsen
#include "scipp/neutron/diffraction/convert_with_calibration.h"
#include "scipp/core/counts.h"
#include "scipp/core/except.h"
#include "scipp/core/groupby.h"

using namespace scipp::core;

namespace scipp::neutron::diffraction {

Dataset convert_with_calibration(Dataset d, Dataset cal) {

  std::vector<Variable> oldBinWidths;
  std::vector<Variable> newBinWidths;

  if (d.coords().contains(Dim::Tof)) {
    // 1. Record ToF bin widths
    OldBinWidths = counts::getBinWidths(d.coords(), {Dim::Tof});
  }

  // 2. There may be a grouping of detectors, in which case we need to apply it
  // to the cal information first.
  if (d.labels().contains("detector_info")) {
    // 2a. Merge cal with detector_info, which contains information on how `d`
    // groups its detectors. At the same time, the coord comparison in `merge`
    // ensures that detector IDs of `d` match those of `cal` .
    const auto &detector_info = d.labels()["detector_info"].value<Dataset>();
    cal = merge(detector_info, cal);

    // 2b. Translate detector-based calibration information into coordinates of
    // data.
    // We are hard-coding some information here: the existence of "spectra",
    // since we require labels named "spectrum" and a corresponding dimension.
    // Given that this in in a branch that is access only if "detector_info" is
    // present this should probably be ok.
    cal = groupby(cal, "spectrum", Dim::Spectrum).mean(Dim::Detector);
  } else if (d.dimensions().count(cal["tzero"].dims().inner()) != 1) {
    throw except::DimensionError("Calibration depends on dimension " +
                                 to_string(cal["tzero"].dims().inner()) +
                                 " that is not present in the converted "
                                 "data " +
                                 to_string(d) +
                                 ". Missing detector information?");
  }

  if (d.coords().contains(Dim::Tof)) {
    // 3. Transform coordinate
    d.setCoord(Dim::Tof, (d.coords()[Dim::Tof] - cal["tzero"].data()) /
                             cal["difc"].data());

    // 4. Record DSpacing bin widths
    newBinWidths = counts::getBinWidths(d.coords(), {Dim::Tof});
  }

  // 5. Transform variables
  for (const auto &[name, data] : d) {
    static_cast<void>(name);
    if (data.coords()[Dim::Tof].dims().sparse()) {
      data.coords()[Dim::Tof].assign(data.coords()[Dim::Tof] -
                                     cal["tzero"].data());
      data.coords()[Dim::Tof].assign(data.coords()[Dim::Tof] /
                                     cal["difc"].data());
    } else if (data.unit().isCountDensity()) {
      counts::fromDensity(data, oldBinWidths);
      counts::toDensity(data, newBinWidths);
    }
  }

  d.rename(Dim::Tof, Dim::DSpacing);
  return d;
}

} // namespace scipp::neutron::diffraction
