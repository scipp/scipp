// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"

#include "scipp/neutron/constants.h"

using namespace scipp;
using namespace scipp::neutron;

namespace mock {

struct Dummy {
  Variable ei;
  Variable ef;
};

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

Variable l1(const Dummy &dummy) {
  return norm(sample_position(dummy) - source_position(dummy));
}

Variable l2(const Dummy &dummy) {
  return norm(position(dummy) - sample_position(dummy));
}

Variable flight_path_length(const Dummy &dummy) {
  return l1(dummy) + l2(dummy);
}

Variable incident_energy(const Dummy &dummy) { return dummy.ei; }
Variable final_energy(const Dummy &dummy) { return dummy.ef; }

} // namespace mock

class ConstantsTest : public ::testing::Test {
protected:
  mock::Dummy dummy;
  const Variable theta = mock::scattering_angle(dummy);
  const Variable L1 = mock::l1(dummy);
  const Variable L2 = mock::l2(dummy);
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

TEST_F(ConstantsTest, tof_to_energy_transfer_fails) {
  EXPECT_THROW(constants::tof_to_energy_transfer(dummy), std::runtime_error);
  dummy.ei = 3.0 * units::meV;
  dummy.ef = 3.0 * units::meV;
  EXPECT_THROW(constants::tof_to_energy_transfer(dummy), std::runtime_error);
}

TEST_F(ConstantsTest, tof_to_energy_transfer_direct) {
  const auto Ei = 3.0 * units::meV;
  dummy.ei = Ei;
  const auto [scale, tof_shift, energy_shift] =
      constants::tof_to_energy_transfer(dummy);
  const auto C = Variable(constants::tof_to_energy_physical_constants);
  EXPECT_EQ(scale, -L2 * L2 * C);
  EXPECT_EQ(tof_shift, sqrt(L1 * L1 * C / Ei));
  EXPECT_EQ(energy_shift, -Ei);
}

TEST_F(ConstantsTest, tof_to_energy_transfer_indirect) {
  const auto Ef = 3.0 * units::meV;
  dummy.ef = Ef;
  const auto [scale, tof_shift, energy_shift] =
      constants::tof_to_energy_transfer(dummy);
  const auto C = Variable(constants::tof_to_energy_physical_constants);
  EXPECT_EQ(scale, L1 * L1 * C);
  EXPECT_EQ(tof_shift, sqrt(L2 * L2 * C / Ef));
  EXPECT_EQ(energy_shift, Ef);
}

TEST_F(ConstantsTest, wavelength_to_q) {
  EXPECT_EQ(constants::wavelength_to_q(dummy),
            sin(theta) * (4.0 * scipp::pi<double> * units::one));
}
