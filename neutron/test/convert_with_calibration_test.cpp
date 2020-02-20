// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/counts.h"
#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"
#include "scipp/neutron/diffraction/convert_with_calibration.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::neutron;

namespace {
Dataset makeTofDataForUnitConversion(const bool dense_coord = true) {
  Dataset tof;

  if (dense_coord)
    tof.setCoord(Dim::Tof, makeVariable<double>(
                               Dims{Dim::Tof}, Shape{4}, units::Unit(units::us),
                               Values{4000, 5000, 6100, 7300}));

  tof.setData("counts",
              makeVariable<double>(Dims{Dim::Spectrum, Dim::Tof}, Shape{2, 3},
                                   Values{1, 2, 3, 4, 5, 6}));
  tof["counts"].data().setUnit(units::counts);

  auto events = makeVariable<double>(Dims{Dim::Spectrum, Dim::Tof},
                                     Shape{2l, Dimensions::Sparse});
  events.setUnit(units::us);
  auto eventLists = events.sparseValues<double>();
  eventLists[0] = {1000, 3000, 2000, 4000};
  eventLists[1] = {5000, 6000, 3000};
  tof.setSparseCoord("events", events);
  tof.setSparseLabels("events", "aux", events);

  return tof;
}

Dataset makeCalTable() {
  Dataset cal;
  cal.setData("tzero",
              makeVariable<double>(Dims{Dim::Spectrum}, Shape{2},
                                   units::Unit(units::us), Values{1.1, 2.2}));
  cal.setData("difc",
              makeVariable<double>(Dims{Dim::Spectrum}, Shape{2},
                                   units::Unit{units::us / units::angstrom},
                                   Values{3.3, 4.4}));
  return cal;
}
} // namespace

TEST(ConvertWithCaliabrationDataArray, data_array) {
  const auto tof = makeTofDataForUnitConversion();
  const auto cal = makeCalTable();

  for (const auto &item : tof) {
    const auto dspacing =
        diffraction::convert_with_calibration(copy(item), cal);
    ASSERT_TRUE(dspacing.coords().contains(Dim::DSpacing));
    ASSERT_EQ(dspacing.coords()[Dim::DSpacing].unit(), scipp::units::angstrom);
  }
}

TEST(ConvertWithCaliabrationDataArray, dataset) {
  const auto tof = makeTofDataForUnitConversion();
  const auto cal = makeCalTable();

  const auto dspacing = diffraction::convert_with_calibration(tof, cal);
  ASSERT_TRUE(dspacing.coords().contains(Dim::DSpacing));
  ASSERT_EQ(dspacing.coords()[Dim::DSpacing].unit(), scipp::units::angstrom);
}
