/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset_index.h"
#include "md_zip_view.h"

TEST(ExampleInstrument, basics) {
  gsl::index ndet = 4;

  Dataset detectors;
  detectors.insert<Coord::DetectorId>({Dim::Detector, ndet}, {1, 2, 3, 4});
  detectors.insert<Coord::Position>({Dim::Detector, ndet}, ndet,
                                    Eigen::Vector3d{0.0, 0.0, 2.0});
  MDZipView<const Coord::DetectorId, Coord::Position> view(detectors);
  for (auto &det : view) {
    det.get<Coord::Position>()[0] = 0.1 * det.get<Coord::DetectorId>();
    EXPECT_EQ(det.get<Coord::Position>()[0],
              0.1 * det.get<Coord::DetectorId>());
  }

  // For const access we need to make sure that the implementation is not
  // attempting to compute derived positions based on detector grouping (which
  // does not exist in this case).
  MDZipView<const Coord::Position> directConstView(detectors);
  // If not implemented correctly this would actually segfault, not throw.
  ASSERT_NO_THROW(directConstView.begin()->get<Coord::Position>());
  EXPECT_EQ(directConstView.begin()->get<Coord::Position>()[0], 0.1);

  Dataset components;
  // Source and sample
  components.insert<Coord::Position>(
      {Dim::Component, 2},
      {Eigen::Vector3d{0.0, 0.0, -10.0}, Eigen::Vector3d{0.0, 0.0, 0.0}});

  Dataset d;
  Vector<boost::container::small_vector<gsl::index, 1>> grouping = {{0, 1},
                                                                    {2, 3}};
  d.insert<Coord::DetectorGrouping>({Dim::Spectrum, 2}, grouping);
  d.insert<Coord::DetectorInfo>({}, {detectors});
  d.insert<Coord::ComponentInfo>({}, {components});

  EXPECT_ANY_THROW(static_cast<void>(MDZipView<Coord::Position>(d)));
  ASSERT_NO_THROW(static_cast<void>(MDZipView<const Coord::Position>(d)));
  MDZipView<const Coord::Position> specPos(d);
  ASSERT_EQ(specPos.size(), 2);
  EXPECT_DOUBLE_EQ(specPos.begin()->get<Coord::Position>()[0], 0.15);
  EXPECT_DOUBLE_EQ((specPos.begin() + 1)->get<Coord::Position>()[0], 0.35);
}
