// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"

#include "scipp/neutron/constants.h"

using namespace scipp;
using namespace scipp::neutron;

namespace mock {

struct Dummy {};

Variable scattering_angle(const Dummy &) { return 0.123 * units::rad; }
} // namespace mock

class ConstantsTest : public ::testing::Test {
protected:
  mock::Dummy dummy;
  const Variable scattering_angle = mock::scattering_angle(dummy);
};

TEST_F(ConstantsTest, wavelength_to_q) {
  EXPECT_EQ(constants::wavelength_to_q(dummy),
            sin(scattering_angle) * (4.0 * scipp::pi<double> * units::one));
}
