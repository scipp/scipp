// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Mads Bertelsen
#include "scipp/neutron/diffraction/convert_with_calibration.h"
#include "scipp/core/counts.h"
#include "scipp/core/dataset_util.h"
#include "scipp/core/except.h"
#include "scipp/core/groupby.h"

using namespace scipp::core;

namespace scipp::neutron::diffraction {

template <class T> bool has_dim(const T &d, const Dim dim) {
  if constexpr (std::is_same_v<T, Dataset>)
    return d.dimensions().count(dim) == 1;
  else
    return d.dims().contains(dim);
}

template <class T> T convert_with_calibration_impl(T d, Dataset cal) {
  for (const auto &item : iter(d))
    if (item.second.hasData())
      expect::notCountDensity(item.second.unit());

  // 1. There may be a grouping of detectors, in which case we need to apply it
  // to the cal information first.
  if (d.labels().contains("detector_info")) {
    // 1a. Merge cal with detector_info, which contains information on how `d`
    // groups its detectors. At the same time, the coord comparison in `merge`
    // ensures that detector IDs of `d` match those of `cal` .
    const auto &detector_info =
        d.labels()["detector_info"].template value<Dataset>();
    cal = merge(detector_info, cal);

    // 1b. Translate detector-based calibration information into coordinates of
    // data.
    // We are hard-coding some information here: the existence of "spectra",
    // since we require labels named "spectrum" and a corresponding dimension.
    // Given that this in in a branch that is access only if "detector_info" is
    // present this should probably be ok.
    cal = groupby(cal, "spectrum", Dim::Spectrum).mean(Dim::Detector);
  } else if (!has_dim(d, cal["tzero"].dims().inner())) {
    throw except::DimensionError("Calibration depends on dimension " +
                                 to_string(cal["tzero"].dims().inner()) +
                                 " that is not present in the converted "
                                 "data " +
                                 to_string(d) +
                                 ". Missing detector information?");
  }

  // 2. Transform coordinate
  if (d.coords().contains(Dim::Tof)) {
    d.setCoord(Dim::Tof, (d.coords()[Dim::Tof] - cal["tzero"].data()) /
                             cal["difc"].data());
  }

  // 3. Transform sparse coordinates
  for (const auto &[name, data] : iter(d)) {
    static_cast<void>(name);
    if (data.coords()[Dim::Tof].dims().sparse()) {
      data.coords()[Dim::Tof].assign(data.coords()[Dim::Tof] -
                                     cal["tzero"].data());
      data.coords()[Dim::Tof].assign(data.coords()[Dim::Tof] /
                                     cal["difc"].data());
    }
  }

  d.rename(Dim::Tof, Dim::DSpacing);
  return d;
}

DataArray convert_with_calibration(DataArray d, Dataset cal) {
  return convert_with_calibration_impl(std::move(d), std::move(cal));
}

Dataset convert_with_calibration(Dataset d, Dataset cal) {
  return convert_with_calibration_impl(std::move(d), std::move(cal));
}

} // namespace scipp::neutron::diffraction
