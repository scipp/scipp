// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"

#include "scipp/neutron/constants.h"

using namespace scipp;
using namespace scipp::neutron;

namespace mock {

struct Dummy {};

Variable source_position(const Dummy &) {
  return makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{1.0, 2.0, 3.0}},
                                       units::m);
}

Variable sample_position(const Dummy &) {
  return makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{2.0, 4.0, 8.0}},
                                       units::m);
}

Variable position(const Dummy &) {
  return makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2},
      Values{Eigen::Vector3d{2.1, 4.1, 8.2}, Eigen::Vector3d{2.2, 4.3, 8.4}},
      units::m);
}

Variable scattering_angle(const Dummy &) {
  // Note: Not related to actual positions.
  return 0.123 * units::rad;
}

Variable flight_path_length(const Dummy &dummy) {
  return norm(sample_position(dummy) - source_position(dummy)) +
         norm((position(dummy) - sample_position(dummy)));
}

} // namespace mock

class ConstantsTest : public ::testing::Test {
protected:
  mock::Dummy dummy;
  const Variable theta = mock::scattering_angle(dummy);
  const Variable L = mock::flight_path_length(dummy);
  const Variable source = mock::source_position(dummy);
  const Variable sample = mock::sample_position(dummy);
  const Variable det = mock::position(dummy);

  Variable normalized_beam() const {
    auto beam = sample - source;
    beam /= norm(beam);
    return beam;
  }

  Variable normalized_scatter() const {
    auto scatter = det - sample;
    scatter /= norm(scatter);
    return scatter;
  }
};

TEST_F(ConstantsTest, tof_to_dspacing) {
  EXPECT_EQ(constants::tof_to_dspacing(dummy),
            reciprocal(L *
                       Variable(constants::tof_to_dspacing_physical_constants *
                                sqrt(0.5)) *
                       sqrt(1.0 * units::one -
                            dot(normalized_beam(), normalized_scatter()))));
}

TEST_F(ConstantsTest, tof_to_wavelength) {
  EXPECT_EQ(constants::tof_to_wavelength(dummy),
            Variable(constants::tof_to_wavelength_physical_constants) / L);
}

TEST_F(ConstantsTest, tof_to_energy) {
  EXPECT_EQ(constants::tof_to_energy(dummy),
            L * L * Variable(constants::tof_to_energy_physical_constants));
}

TEST_F(ConstantsTest, wavelength_to_q) {
  EXPECT_EQ(constants::wavelength_to_q(dummy),
            sin(theta) * (4.0 * scipp::pi<double> * units::one));
}
