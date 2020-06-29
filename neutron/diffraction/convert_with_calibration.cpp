// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Mads Bertelsen
#include "scipp/neutron/diffraction/convert_with_calibration.h"
#include "scipp/dataset/counts.h"
#include "scipp/dataset/dataset_util.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/groupby.h"
#include "scipp/variable/operations.h"

using namespace scipp::variable;
using namespace scipp::dataset;

namespace scipp::neutron::diffraction {

template <class T> bool has_dim(const T &d, const Dim dim) {
  if constexpr (std::is_same_v<T, Dataset>)
    return d.dimensions().count(dim) == 1;
  else
    return d.dims().contains(dim);
}

template <class T> T convert_with_calibration_impl(T d, Dataset cal) {
  for (const auto &item : iter(d))
    if (item.hasData())
      core::expect::notCountDensity(item.unit());

  // 1. There may be a grouping of detectors, in which case we need to apply it
  // to the cal information first.
  if (d.coords().contains(Dim("detector-info"))) {
    // 1a. Merge cal with detector-info, which contains information on how `d`
    // groups its detectors. At the same time, the coord comparison in `merge`
    // ensures that detector IDs of `d` match those of `cal` .
    const auto &detector_info =
        d.coords()[Dim("detector-info")].template value<Dataset>();
    cal = merge(detector_info, cal);
    // Masking and grouping information in the calibration table interferes with
    // `groupby.mean`, dropping.
    for (const auto &name : {"mask", "group"})
      if (cal.contains(name))
        cal.erase(name);

    // 1b. Translate detector-based calibration information into coordinates of
    // data.
    // We are hard-coding some information here: the existence of "spectra",
    // since we require labels named "spectrum" and a corresponding dimension.
    // Given that this is in a branch that is access only if "detector-info" is
    // present this should probably be ok.
    cal = groupby(cal, Dim("spectrum")).mean(Dim::Detector);
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
    if (contains_events(d.coords()[Dim::Tof])) {
      d.coords()[Dim::Tof] -= cal["tzero"].data();
      d.coords()[Dim::Tof] *= reciprocal(cal["difc"].data());
    } else {
      d.coords().set(Dim::Tof, (d.coords()[Dim::Tof] - cal["tzero"].data()) /
                                   cal["difc"].data());
    }
  }

  // 3. Transform realigned items
  for (const auto &item : iter(d))
    if (item.unaligned() && contains_events(item.unaligned())) {
      item.unaligned().coords()[Dim::Tof] -= cal["tzero"].data();
      item.unaligned().coords()[Dim::Tof] *= reciprocal(cal["difc"].data());
    }

  d.rename(Dim::Tof, Dim::DSpacing);
  return d;
}

void validate_calibration(const Dataset &cal) {
  if (cal["tzero"].unit() != scipp::units::us)
    throw except::UnitError("tzero must have units of `us`");
  if (cal["difc"].unit() != scipp::units::us / scipp::units::angstrom)
    throw except::UnitError("difc must have units of `us / angstrom`");
}

DataArray convert_with_calibration(DataArray d, Dataset cal) {
  validate_calibration(cal);
  return convert_with_calibration_impl(std::move(d), std::move(cal));
}

Dataset convert_with_calibration(Dataset d, Dataset cal) {
  validate_calibration(cal);
  return convert_with_calibration_impl(std::move(d), std::move(cal));
}

} // namespace scipp::neutron::diffraction
