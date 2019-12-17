// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"
#include "scipp/neutron/beamline.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::neutron;

namespace {
static const auto source_pos = Eigen::Vector3d{0.0, 0.0, -9.99};
static const auto sample_pos = Eigen::Vector3d{0.0, 0.0, 0.01};
} // namespace

Dataset makeDatasetWithBeamline() {
  Dataset beamline;
  Dataset components;
  // Source and sample
  components.setData("position",
                     makeVariable<Eigen::Vector3d>(
                         Dims{Dim::Row}, Shape{2}, units::Unit(units::m),
                         Values{source_pos, sample_pos}));
  beamline.setLabels(
      "source_position",
      makeVariable<Eigen::Vector3d>(units::Unit(units::m), Values{source_pos}));
  beamline.setLabels(
      "sample_position",
      makeVariable<Eigen::Vector3d>(units::Unit(units::m), Values{sample_pos}));
  // TODO Need fuzzy comparison for variables to write a convenient test with
  // detectors away from the axes.
  beamline.setLabels("position",
                     makeVariable<Eigen::Vector3d>(
                         Dims{Dim::Spectrum}, Shape{2}, units::Unit(units::m),
                         Values{Eigen::Vector3d{1.0, 0.0, 0.01},
                                Eigen::Vector3d{0.0, 1.0, 0.01}}));
  return beamline;
}

class BeamlineTest : public ::testing::Test {
protected:
  const Dataset dataset{makeDatasetWithBeamline()};
};

TEST_F(BeamlineTest, basics) {
  ASSERT_EQ(source_position(dataset),
            makeVariable<Eigen::Vector3d>(
                Dims(), Shape(), units::Unit(units::m), Values{source_pos}));
  ASSERT_EQ(sample_position(dataset),
            makeVariable<Eigen::Vector3d>(
                Dims(), Shape(), units::Unit(units::m), Values{sample_pos}));
  ASSERT_EQ(l1(dataset),
            makeVariable<double>(Dims(), Shape(), units::Unit(units::m),
                                 Values{10.0}));
}

TEST_F(BeamlineTest, l2) {
  ASSERT_EQ(l2(dataset),
            makeVariable<double>(Dims{Dim::Spectrum}, Shape{2},
                                 units::Unit(units::m), Values{1.0, 1.0}));
}

template <class T> constexpr T pi = T(3.1415926535897932385L);

TEST_F(BeamlineTest, scattering_angle) {
  ASSERT_EQ(two_theta(dataset),
            makeVariable<double>(Dims{Dim::Spectrum}, Shape{2},
                                 units::Unit(units::rad),
                                 Values{pi<double> / 2, pi<double> / 2}));
  ASSERT_EQ(scattering_angle(dataset), 0.5 * two_theta(dataset));
}
