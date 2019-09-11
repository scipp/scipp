// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/counts.h"
#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"
#include "scipp/neutron/convert.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::neutron;

Dataset makeTofDataForUnitConversion() {
  Dataset tof;

  tof.setCoord(Dim::Tof, makeVariable<double>({Dim::Tof, 4}, units::us,
                                              {4000, 5000, 6100, 7300}));

  Dataset components;
  // Source and sample
  components.setData("position", makeVariable<Eigen::Vector3d>(
                                     {Dim::Row, 2}, units::m,
                                     {Eigen::Vector3d{0.0, 0.0, -10.0},
                                      Eigen::Vector3d{0.0, 0.0, 0.0}}));
  tof.setLabels("component_info", makeVariable<Dataset>(components));
  tof.setCoord(Dim::Position,
               makeVariable<Eigen::Vector3d>({Dim::Position, 2}, units::m,
                                             {Eigen::Vector3d{1.0, 0.0, 0.0},
                                              Eigen::Vector3d{0.1, 0.0, 1.0}}));

  tof.setData("counts",
              makeVariable<double>({{Dim::Position, 2}, {Dim::Tof, 3}},
                                   {1, 2, 3, 4, 5, 6}));
  tof["counts"].data().setUnit(units::counts);

  auto events =
      makeVariable<double>({Dim::Position, Dim::Tof}, {2, Dimensions::Sparse});
  events.setUnit(units::us);
  auto eventLists = events.sparseValues<double>();
  eventLists[0] = {1000, 3000, 2000, 4000};
  eventLists[1] = {5000, 6000, 3000};
  tof.setSparseCoord("events", std::move(events));

  tof.setData("density",
              makeVariable<double>({{Dim::Position, 2}, {Dim::Tof, 3}},
                                   {1, 2, 3, 4, 5, 6}));
  tof["density"].data().setUnit(units::counts / units::us);

  return tof;
}

TEST(Convert, Tof_to_DSpacing) {
  Dataset tof = makeTofDataForUnitConversion();

  auto dspacing = convert(tof, Dim::Tof, Dim::DSpacing);

  EXPECT_EQ(dspacing["counts"].dims(),
            Dimensions({{Dim::Position, 2}, {Dim::DSpacing, 3}}));

  ASSERT_THROW(dspacing.coords()[Dim::Tof], std::out_of_range);
  ASSERT_NO_THROW(dspacing.coords()[Dim::DSpacing]);

  const auto &coord = dspacing.coords()[Dim::DSpacing];
  // Due to conversion, the coordinate now also depends on Dim::Spectrum.
  ASSERT_EQ(coord.dims(), Dimensions({{Dim::Position, 2}, {Dim::DSpacing, 4}}));
  EXPECT_EQ(coord.unit(), units::angstrom);

  const auto values = coord.values<double>();
  // Rule of thumb (https://www.psi.ch/niag/neutron-physics):
  // v [m/s] = 3956 / \lambda [ Angstrom ]
  Variable tof_in_seconds = tof.coords()[Dim::Tof] * 1e-6;
  const auto tofs = tof_in_seconds.values<double>();
  // Spectrum 0 is 11 m from source
  // 2d sin(theta) = n \lambda
  // theta = 45 deg => d = lambda / (2 * 1 / sqrt(2))
  EXPECT_NEAR(values[0], 3956.0 / (11.0 / tofs[0]) / sqrt(2.0),
              values[0] * 1e-3);
  EXPECT_NEAR(values[1], 3956.0 / (11.0 / tofs[1]) / sqrt(2.0),
              values[1] * 1e-3);
  EXPECT_NEAR(values[2], 3956.0 / (11.0 / tofs[2]) / sqrt(2.0),
              values[2] * 1e-3);
  EXPECT_NEAR(values[3], 3956.0 / (11.0 / tofs[3]) / sqrt(2.0),
              values[3] * 1e-3);
  // Spectrum 1
  // sin(2 theta) = 0.1/(L-10)
  const double L = 10.0 + sqrt(1.0 * 1.0 + 0.1 * 0.1);
  const double lambda_to_d = 1.0 / (2.0 * sin(0.5 * asin(0.1 / (L - 10.0))));
  EXPECT_NEAR(values[4], 3956.0 / (L / tofs[0]) * lambda_to_d,
              values[4] * 1e-3);
  EXPECT_NEAR(values[5], 3956.0 / (L / tofs[1]) * lambda_to_d,
              values[5] * 1e-3);
  EXPECT_NEAR(values[6], 3956.0 / (L / tofs[2]) * lambda_to_d,
              values[6] * 1e-3);
  EXPECT_NEAR(values[7], 3956.0 / (L / tofs[3]) * lambda_to_d,
              values[7] * 1e-3);

  ASSERT_TRUE(dspacing.contains("counts"));
  const auto &data = dspacing["counts"];
  ASSERT_EQ(data.dims(), Dimensions({{Dim::Position, 2}, {Dim::DSpacing, 3}}));
  EXPECT_TRUE(equals(data.values<double>(), {1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(data.unit(), units::counts);

  ASSERT_TRUE(dspacing.contains("events"));
  const auto &events = dspacing["events"];
  ASSERT_EQ(events.dims(), Dimensions({Dim::Position, Dim::DSpacing},
                                      {2, Dimensions::Sparse}));
  const auto &tof0 = tof["events"].coords()[Dim::Tof].sparseValues<double>()[0];
  const auto &d0 = events.coords()[Dim::DSpacing].sparseValues<double>()[0];
  ASSERT_EQ(scipp::size(d0), 4);
  EXPECT_NEAR(d0[0], 3956.0 / (1e6 * 11.0 / tof0[0]) / sqrt(2.0), d0[0] * 1e-3);
  EXPECT_NEAR(d0[1], 3956.0 / (1e6 * 11.0 / tof0[1]) / sqrt(2.0), d0[1] * 1e-3);
  EXPECT_NEAR(d0[2], 3956.0 / (1e6 * 11.0 / tof0[2]) / sqrt(2.0), d0[2] * 1e-3);
  EXPECT_NEAR(d0[3], 3956.0 / (1e6 * 11.0 / tof0[3]) / sqrt(2.0), d0[3] * 1e-3);
  const auto &tof1 = tof["events"].coords()[Dim::Tof].sparseValues<double>()[1];
  const auto &d1 = events.coords()[Dim::DSpacing].sparseValues<double>()[1];
  ASSERT_EQ(scipp::size(d1), 3);
  EXPECT_NEAR(d1[0], 3956.0 / (1e6 * 11.0 / tof1[0]) * lambda_to_d,
              d1[0] * 1e-3);
  EXPECT_NEAR(d1[1], 3956.0 / (1e6 * 11.0 / tof1[1]) * lambda_to_d,
              d1[1] * 1e-3);
  EXPECT_NEAR(d1[2], 3956.0 / (1e6 * 11.0 / tof1[2]) * lambda_to_d,
              d1[2] * 1e-3);

  ASSERT_TRUE(dspacing.contains("density"));
  const auto &density = dspacing["density"];
  ASSERT_EQ(density.dims(),
            Dimensions({{Dim::Position, 2}, {Dim::DSpacing, 3}}));
  EXPECT_EQ(density.unit(), units::counts / units::angstrom);
  const auto vals = density.values<double>();
  EXPECT_FALSE(equals(vals, {1, 2, 3, 4, 5, 6}));
  // Spectrum 0
  EXPECT_DOUBLE_EQ(vals[0], 1.0 * 1000 / (values[1] - values[0]));
  EXPECT_DOUBLE_EQ(vals[1], 2.0 * 1100 / (values[2] - values[1]));
  EXPECT_DOUBLE_EQ(vals[2], 3.0 * 1200 / (values[3] - values[2]));
  // Spectrum 1
  EXPECT_DOUBLE_EQ(vals[3], 4.0 * 1000 / (values[5] - values[4]));
  EXPECT_DOUBLE_EQ(vals[4], 5.0 * 1100 / (values[6] - values[5]));
  EXPECT_DOUBLE_EQ(vals[5], 6.0 * 1200 / (values[7] - values[6]));

  ASSERT_EQ(dspacing.coords()[Dim::Position], tof.coords()[Dim::Position]);
  ASSERT_EQ(dspacing.labels()["component_info"],
            tof.labels()["component_info"]);
}

TEST(Convert, DSpacing_to_Tof) {
  /* Assuming the Tof_to_DSpacing test is correct and passing we can test the
   * inverse conversion by simply comparing a round trip conversion with the
   * original data. */

  const Dataset tof_original = makeTofDataForUnitConversion();
  const auto dspacing = convert(tof_original, Dim::Tof, Dim::DSpacing);
  const auto tof = convert(dspacing, Dim::DSpacing, Dim::Tof);

  /* Test coordinates */
  /* Broadcasting is needed as conversion introduces the dependance on
   * Dim::Position */
  EXPECT_EQ(tof.coords()[Dim::Tof], broadcast(tof_original.coords()[Dim::Tof],
                                              tof.coords()[Dim::Tof].dims()));

  /* Test sparse/event data */
  ASSERT_TRUE(tof.contains("events"));
  const auto events = tof["events"].coords()[Dim::Tof].sparseValues<double>();
  const auto events_original =
      tof_original["events"].coords()[Dim::Tof].sparseValues<double>();
  EXPECT_TRUE(equals(events[0], events_original[0], 1e-15));
  EXPECT_TRUE(equals(events[1], events_original[1], 1e-12));

  /* Test count density data */
  ASSERT_TRUE(tof.contains("density"));
  EXPECT_EQ(tof["density"].data(), tof_original["density"].data());
}

/*
TEST(Dataset, convert) {
  Dataset tof = makeTofDataForUnitConversion();

  auto energy = convert(tof, Dim::Tof, Dim::Energy);

  ASSERT_FALSE(energy.dimensions().contains(Dim::Tof));
  ASSERT_TRUE(energy.dimensions().contains(Dim::Energy));
  EXPECT_EQ(energy.dimensions()[Dim::Energy], 3);

  ASSERT_FALSE(energy.contains(Coord::Tof));
  ASSERT_TRUE(energy.contains(Coord::Energy));
  const auto &coord = energy(Coord::Energy);
  // Due to conversion, the coordinate now also depends on Dim::Spectrum.
  ASSERT_EQ(coord.dimensions(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::Energy, 4}}));
  EXPECT_EQ(coord.unit(), units::meV);

  const auto values = coord.span<double>();
  // Rule of thumb (https://www.psi.ch/niag/neutron-physics):
  // v [m/s] = 437 * sqrt ( E[meV] )
  Variable tof_in_seconds = tof(Coord::Tof) * 1e-6;
  const auto tofs = tof_in_seconds.span<double>();
  // Spectrum 0 is 11 m from source
  EXPECT_NEAR(values[0], pow((11.0 / tofs[0]) / 437.0, 2), values[0] * 0.01);
  EXPECT_NEAR(values[1], pow((11.0 / tofs[1]) / 437.0, 2), values[1] * 0.01);
  EXPECT_NEAR(values[2], pow((11.0 / tofs[2]) / 437.0, 2), values[2] * 0.01);
  EXPECT_NEAR(values[3], pow((11.0 / tofs[3]) / 437.0, 2), values[3] * 0.01);
  // Spectrum 1
  const double L = 10.0 + sqrt(1.0 * 1.0 + 0.1 * 0.1);
  EXPECT_NEAR(values[4], pow((L / tofs[0]) / 437.0, 2), values[4] * 0.01);
  EXPECT_NEAR(values[5], pow((L / tofs[1]) / 437.0, 2), values[5] * 0.01);
  EXPECT_NEAR(values[6], pow((L / tofs[2]) / 437.0, 2), values[6] * 0.01);
  EXPECT_NEAR(values[7], pow((L / tofs[3]) / 437.0, 2), values[7] * 0.01);

  ASSERT_TRUE(energy.contains(Data::Value, "counts"));
  const auto &data = energy(Data::Value, "counts");
  ASSERT_EQ(data.dimensions(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::Energy, 3}}));
  EXPECT_TRUE(equals(data.span<double>(), {1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(data.unit(), units::counts);

  ASSERT_TRUE(energy.contains(Data::Value, "counts/us"));
  const auto &density = energy(Data::Value, "counts/us");
  ASSERT_EQ(density.dimensions(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::Energy, 3}}));
  EXPECT_FALSE(equals(density.span<double>(), {1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(density.unit(), units::counts / units::meV);

  ASSERT_TRUE(energy.contains(Coord::Position));
  ASSERT_TRUE(energy.contains(Coord::ComponentInfo));
}

TEST(Dataset, convert_to_energy_fails_for_inelastic) {
  Dataset tof = makeTofDataForUnitConversion();

  // Note these conversion fail only because they are not implemented. It should
  // definitely be possible to support this.

  tof.insert(Coord::Ei, makeVariable<double>({}, units::meV, {1}));
  EXPECT_THROW_MSG(convert(tof, Dim::Tof, Dim::Energy), std::runtime_error,
                   "Dataset contains Coord::Ei or Coord::Ef. However, "
                   "conversion to Dim::Energy is currently only supported for "
                   "elastic scattering.");
  tof.erase(Coord::Ei);

  tof.insert(Coord::Ef, {Dim::Spectrum, 2}, {1.0, 1.5});
  EXPECT_THROW_MSG(convert(tof, Dim::Tof, Dim::Energy), std::runtime_error,
                   "Dataset contains Coord::Ei or Coord::Ef. However, "
                   "conversion to Dim::Energy is currently only supported for "
                   "elastic scattering.");
  tof.erase(Coord::Ef);

  EXPECT_NO_THROW(convert(tof, Dim::Tof, Dim::Energy));
}

TEST(Dataset, convert_direct_inelastic) {
  Dataset tof;

  tof.insert(Coord::Tof,
             makeVariable<double>({Dim::Tof, 4}, units::us, {1, 2, 3, 4}));

  Dataset components;
  // Source and sample
  components.insert(Coord::Position, makeVariable<Eigen::Vector3d>(
                                         {Dim::Component, 2}, units::m,
                                         {Eigen::Vector3d{0.0, 0.0, -10.0},
                                          Eigen::Vector3d{0.0, 0.0, 0.0}}));
  tof.insert(Coord::ComponentInfo, {}, {components});
  tof.insert(Coord::Position,
             makeVariable<Eigen::Vector3d>({Dim::Spectrum, 3}, units::m,
                                           {Eigen::Vector3d{0.0, 0.0, 1.0},
                                            Eigen::Vector3d{0.0, 0.0, 1.0},
                                            Eigen::Vector3d{0.1, 0.0, 1.0}}));

  tof.insert(Data::Value, "", {{Dim::Spectrum, 3}, {Dim::Tof, 3}},
             {1, 2, 3, 4, 5, 6, 7, 8, 9});
  tof(Data::Value, "").setUnit(units::counts);

  tof.insert(Coord::Ei, makeVariable<double>({}, units::meV, {1}));

  auto energy = convert(tof, Dim::Tof, Dim::DeltaE);

  ASSERT_FALSE(energy.dimensions().contains(Dim::Tof));
  ASSERT_TRUE(energy.dimensions().contains(Dim::DeltaE));
  EXPECT_EQ(energy.dimensions()[Dim::DeltaE], 3);

  ASSERT_FALSE(energy.contains(Coord::Tof));
  ASSERT_TRUE(energy.contains(Coord::DeltaE));
  const auto &coord = energy(Coord::DeltaE);
  // Due to conversion, the coordinate now also depends on Dim::Spectrum.
  ASSERT_EQ(coord.dimensions(),
            Dimensions({{Dim::Spectrum, 3}, {Dim::DeltaE, 4}}));
  // TODO Check actual values here after conversion is fixed.
  EXPECT_FALSE(
      equals(coord.span<double>(), {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4}));
  // 2 spectra at same position see same deltaE.
  EXPECT_EQ(coord(Dim::Spectrum, 0).span<double>()[0],
            coord(Dim::Spectrum, 1).span<double>()[0]);
  EXPECT_EQ(coord.unit(), units::meV);

  ASSERT_TRUE(energy.contains(Data::Value));
  const auto &data = energy(Data::Value);
  ASSERT_EQ(data.dimensions(),
            Dimensions({{Dim::Spectrum, 3}, {Dim::DeltaE, 3}}));
  EXPECT_TRUE(equals(data.span<double>(), {3, 2, 1, 6, 5, 4, 9, 8, 7}));
  EXPECT_EQ(data.unit(), units::counts);

  ASSERT_TRUE(energy.contains(Coord::Position));
  ASSERT_TRUE(energy.contains(Coord::ComponentInfo));
  ASSERT_TRUE(energy.contains(Coord::Ei));
}

Dataset makeMultiEiTofData() {
  Dataset tof;
  tof.insert(Coord::Tof, makeVariable<double>({Dim::Tof, 4}, units::us,
                                              {1000, 2000, 3000, 4000}));

  Dataset components;
  // Source and sample
  components.insert(Coord::Position, makeVariable<Eigen::Vector3d>(
                                         {Dim::Component, 2}, units::m,
                                         {Eigen::Vector3d{0.0, 0.0, -10.0},
                                          Eigen::Vector3d{0.0, 0.0, 0.0}}));
  tof.insert(Coord::ComponentInfo, {}, {components});
  tof.insert(Coord::Position,
             makeVariable<Eigen::Vector3d>({Dim::Position, 3}, units::m,
                                           {Eigen::Vector3d{0.0, 0.0, 1.0},
                                            Eigen::Vector3d{0.0, 0.0, 1.0},
                                            Eigen::Vector3d{0.1, 0.0, 1.0}}));

  tof.insert(Data::Value, "", {{Dim::Position, 3}, {Dim::Tof, 3}},
             {1, 2, 3, 4, 5, 6, 7, 8, 9});
  tof(Data::Value, "").setUnit(units::counts);

  // In practice not every spectrum would have a different Ei, more likely we
  // would have an extra dimension, Dim::Ei in addition to Dim::Position.
  tof.insert(Coord::Ei, makeVariable<double>({Dim::Position, 3}, units::meV,
                                             {10.0, 10.5, 11.0}));
  return tof;
}

TEST(Dataset, convert_direct_inelastic_multi_Ei) {
  const auto tof = makeMultiEiTofData();

  auto energy = convert(tof, Dim::Tof, Dim::DeltaE);

  ASSERT_FALSE(energy.dimensions().contains(Dim::Tof));
  ASSERT_TRUE(energy.dimensions().contains(Dim::DeltaE));
  EXPECT_EQ(energy.dimensions()[Dim::DeltaE], 3);

  ASSERT_FALSE(energy.contains(Coord::Tof));
  ASSERT_TRUE(energy.contains(Coord::DeltaE));
  const auto &coord = energy(Coord::DeltaE);
  // Due to conversion, the coordinate now also depends on Dim::Position.
  ASSERT_EQ(coord.dimensions(),
            Dimensions({{Dim::Position, 3}, {Dim::DeltaE, 4}}));
  // TODO Check actual values here after conversion is fixed.
  EXPECT_FALSE(
      equals(coord.span<double>(), {1000, 2000, 3000, 4000, 1000, 2000, 3000,
                                    4000, 1000, 2000, 3000, 4000}));
  // 2 spectra at same position, but now their Ei differs, so deltaE is also
  // different (compare to test for single Ei above).
  EXPECT_NE(coord(Dim::Position, 0).span<double>()[0],
            coord(Dim::Position, 1).span<double>()[0]);
  EXPECT_EQ(coord.unit(), units::meV);

  ASSERT_TRUE(energy.contains(Data::Value));
  const auto &data = energy(Data::Value);
  ASSERT_EQ(data.dimensions(),
            Dimensions({{Dim::Position, 3}, {Dim::DeltaE, 3}}));
  EXPECT_TRUE(equals(data.span<double>(), {3, 2, 1, 6, 5, 4, 9, 8, 7}));
  EXPECT_EQ(data.unit(), units::counts);

  ASSERT_TRUE(energy.contains(Coord::Position));
  ASSERT_TRUE(energy.contains(Coord::ComponentInfo));
  ASSERT_TRUE(energy.contains(Coord::Ei));
}
*/
