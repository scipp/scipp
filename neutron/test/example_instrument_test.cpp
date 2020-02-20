// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset_index.h"
#include "md_zip_view.h"

using namespace scipp::core;

TEST(ExampleInstrument, basics) {
  scipp::index ndet = 4;

  Dataset detectors;
  detectors.insert(Coord::DetectorId, {Dim::Detector, ndet}, {1, 2, 3, 4});
  detectors.insert(Coord::Position, {Dim::Detector, ndet}, ndet,
                   Eigen::Vector3d{0.0, 0.0, 2.0});
  auto view =
      zipMD(detectors, MDRead(Coord::DetectorId), MDWrite(Coord::Position));
  for (auto &det : view) {
    det.get(Coord::Position)[0] = 0.1 * det.get(Coord::DetectorId);
    EXPECT_EQ(det.get(Coord::Position)[0], 0.1 * det.get(Coord::DetectorId));
  }

  // For const access we need to make sure that the implementation is not
  // attempting to compute derived positions based on detector grouping (which
  // does not exist in this case).
  auto directConstView = zipMD(detectors, MDRead(Coord::Position));
  // If not implemented correctly this would actually segfault, not throw.
  ASSERT_NO_THROW(directConstView.begin()->get(Coord::Position));
  EXPECT_EQ(directConstView.begin()->get(Coord::Position)[0], 0.1);

  Dataset components;
  // Source and sample
  components.insert(
      Coord::Position, {Dim::Component, 2},
      {Eigen::Vector3d{0.0, 0.0, -10.0}, Eigen::Vector3d{0.0, 0.0, 0.0}});

  Dataset d;
  d.insert(Coord::DetectorGrouping, {Dim::Spectrum, 2}, {{0, 1}, {2, 3}});
  d.insert(Coord::DetectorInfo, {}, {detectors});
  d.insert(Coord::ComponentInfo, {}, {components});

  EXPECT_ANY_THROW(zipMD(d, MDWrite(Coord::Position)));
  ASSERT_NO_THROW(zipMD(d, MDRead(Coord::Position)));
  auto specPos = zipMD(d, MDRead(Coord::Position));
  ASSERT_EQ(specPos.size(), 2);
  EXPECT_DOUBLE_EQ(specPos.begin()->get(Coord::Position)[0], 0.15);
  EXPECT_DOUBLE_EQ((specPos.begin() + 1)->get(Coord::Position)[0], 0.35);
}
