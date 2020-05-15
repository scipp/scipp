// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/neutron/conversions.h"

using namespace scipp;
using namespace scipp::neutron;

class ConversionsTest : public ::testing::Test {
protected:
  const double coord = 1.2345;
  const double alpha = 4.56;
};

TEST_F(ConversionsTest, tof_to_energy) {
  auto inout = coord;
  conversions::tof_to_energy(inout, alpha);
  EXPECT_EQ(inout, alpha / (coord * coord));
}

TEST_F(ConversionsTest, energy_to_tof) {
  auto inout = coord;
  conversions::energy_to_tof(inout, alpha);
  EXPECT_EQ(inout, std::sqrt(alpha / coord));
}

TEST_F(ConversionsTest, energy_tof_roundtrip) {
  auto inout = coord;
  conversions::energy_to_tof(inout, alpha);
  conversions::tof_to_energy(inout, alpha);
  EXPECT_EQ(inout, coord);
}

TEST_F(ConversionsTest, wavelength_to_q) {
  auto inout = coord;
  conversions::wavelength_to_q(inout, alpha);
  EXPECT_EQ(inout, alpha / coord);
}

TEST_F(ConversionsTest, wavelength_q_roundtrip) {
  auto inout = coord;
  conversions::wavelength_to_q(inout, alpha);
  conversions::wavelength_to_q(inout, alpha);
  EXPECT_EQ(inout, coord);
}
