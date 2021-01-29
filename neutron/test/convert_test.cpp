// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/counts.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/neutron/convert.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::neutron;

Dataset makeBeamline() {
  Dataset tof;
  static const auto source_pos = Eigen::Vector3d{0.0, 0.0, -10.0};
  static const auto sample_pos = Eigen::Vector3d{0.0, 0.0, 0.0};
  tof.setCoord(Dim("source-position"),
               makeVariable<Eigen::Vector3d>(units::m, Values{source_pos}));
  tof.setCoord(Dim("sample-position"),
               makeVariable<Eigen::Vector3d>(units::m, Values{sample_pos}));

  tof.setCoord(Dim("position"), makeVariable<Eigen::Vector3d>(
                                    Dims{Dim::Spectrum}, Shape{2}, units::m,
                                    Values{Eigen::Vector3d{1.0, 0.0, 0.0},
                                           Eigen::Vector3d{0.1, 0.0, 1.0}}));
  return tof;
}

Dataset makeTofDataset() {
  Dataset tof = makeBeamline();
  tof.setCoord(Dim::Tof,
               makeVariable<double>(Dims{Dim::Tof}, Shape{4}, units::us,
                                    Values{4000, 5000, 6100, 7300}));
  tof.setData("counts",
              makeVariable<double>(Dims{Dim::Spectrum, Dim::Tof}, Shape{2, 3},
                                   units::counts, Values{1, 2, 3, 4, 5, 6}));

  return tof;
}

Variable makeTofBucketedEvents() {
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::Spectrum}, Shape{2}, Values{std::pair{0, 4}, std::pair{4, 7}});
  Variable tofs =
      makeVariable<double>(Dims{Dim::Event}, Shape{7}, units::us,
                           Values{1000, 3000, 2000, 4000, 5000, 6000, 3000});
  Variable weights =
      makeVariable<double>(Dims{Dim::Event}, Shape{7}, Values{}, Variances{});
  DataArray buffer = DataArray(weights, {{Dim::Tof, tofs}});
  return make_bins(std::move(indices), Dim::Event, std::move(buffer));
}

Variable makeCountDensityData(const units::Unit &unit) {
  return makeVariable<double>(Dims{Dim::Spectrum, Dim::Tof}, Shape{2, 3},
                              units::counts / unit, Values{1, 2, 3, 4, 5, 6});
}

class ConvertTest : public testing::TestWithParam<Dataset> {};

INSTANTIATE_TEST_SUITE_P(SingleEntryDataset, ConvertTest,
                         testing::Values(makeTofDataset()));

// Tests for DataArray (or its view) as input, comparing against conversion of
// Dataset.
TEST_P(ConvertTest, DataArray_from_tof) {
  Dataset tof = GetParam();
  for (const auto &dim : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
    const auto expected = convert(tof, Dim::Tof, dim);
    Dataset result;
    for (const auto &data : tof)
      result.setData(data.name(), convert(data, Dim::Tof, dim));
    for (const auto &data : result)
      EXPECT_EQ(data, expected[data.name()]);
  }
}

TEST_P(ConvertTest, DataArray_to_tof) {
  Dataset tof = GetParam();
  for (const auto &dim : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
    const auto input = convert(tof, Dim::Tof, dim);
    const auto expected = convert(input, dim, Dim::Tof);
    Dataset result;
    for (const auto &data : input)
      result.setData(data.name(), convert(data, dim, Dim::Tof));
    for (const auto &data : result)
      EXPECT_EQ(data, expected[data.name()]);
  }
}

TEST_P(ConvertTest, DataArray_non_tof) {
  Dataset tof = GetParam();
  for (const auto &from : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
    const auto input = convert(tof, Dim::Tof, from);
    for (const auto &to : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
      const auto expected = convert(tof, Dim::Tof, to);
      Dataset result;
      for (const auto &data : input)
        result.setData(data.name(), convert(data, from, to));
      for (const auto &data : result)
        EXPECT_TRUE(all(is_approx(data.coords()[to], expected.coords()[to],
                                  1e-9 * expected.coords()[to].unit()))
                        .value<bool>());
    }
  }
}

TEST_P(ConvertTest, convert_slice) {
  Dataset tof = GetParam();
  const auto slice = Slice{Dim::Spectrum, 0};
  // Note: Converting slics of data*sets* not supported right now, since meta
  // data handling implementation in `convert` is current based on dataset
  // coords, but slicing converts this into attrs of *items*.
  for (const auto &dim : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
    EXPECT_EQ(convert(tof["counts"].slice(slice), Dim::Tof, dim),
              convert(tof["counts"], Dim::Tof, dim).slice(slice));
  }
}

TEST_P(ConvertTest, fail_count_density) {
  const Dataset tof = GetParam();
  for (const Dim dim : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
    Dataset a = tof;
    Dataset b = convert(a, Dim::Tof, dim);
    EXPECT_NO_THROW(convert(a, Dim::Tof, dim));
    EXPECT_NO_THROW(convert(b, dim, Dim::Tof));
    a.setData("", makeCountDensityData(a.coords()[Dim::Tof].unit()));
    b.setData("", makeCountDensityData(b.coords()[dim].unit()));
    EXPECT_THROW(convert(a, Dim::Tof, dim), except::UnitError);
    EXPECT_THROW(convert(b, dim, Dim::Tof), except::UnitError);
  }
}

TEST_P(ConvertTest, Tof_to_DSpacing) {
  Dataset tof = GetParam();

  auto dspacing = convert(tof, Dim::Tof, Dim::DSpacing);

  ASSERT_FALSE(dspacing.coords().contains(Dim::Tof));
  ASSERT_TRUE(dspacing.coords().contains(Dim::DSpacing));

  const auto &coord = dspacing.coords()[Dim::DSpacing];

  // Spectrum 1
  // sin(2 theta) = 0.1/(L-10)
  const double L = 10.0 + sqrt(1.0 * 1.0 + 0.1 * 0.1);
  const double lambda_to_d = 1.0 / (2.0 * sin(0.5 * asin(0.1 / (L - 10.0))));

  ASSERT_TRUE(dspacing.contains("counts"));
  EXPECT_EQ(dspacing["counts"].dims(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::DSpacing, 3}}));
  // Due to conversion, the coordinate now also depends on Dim::Spectrum.
  ASSERT_EQ(coord.dims(), Dimensions({{Dim::Spectrum, 2}, {Dim::DSpacing, 4}}));
  EXPECT_EQ(coord.unit(), units::angstrom);

  const auto values = coord.values<double>();
  // Rule of thumb (https://www.psi.ch/niag/neutron-physics):
  // v [m/s] = 3956 / \lambda [ Angstrom ]
  Variable tof_in_seconds = tof.coords()[Dim::Tof] * (1e-6 * units::one);
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
  EXPECT_NEAR(values[4], 3956.0 / (L / tofs[0]) * lambda_to_d,
              values[4] * 1e-3);
  EXPECT_NEAR(values[5], 3956.0 / (L / tofs[1]) * lambda_to_d,
              values[5] * 1e-3);
  EXPECT_NEAR(values[6], 3956.0 / (L / tofs[2]) * lambda_to_d,
              values[6] * 1e-3);
  EXPECT_NEAR(values[7], 3956.0 / (L / tofs[3]) * lambda_to_d,
              values[7] * 1e-3);

  const auto &data = dspacing["counts"];
  ASSERT_EQ(data.dims(), Dimensions({{Dim::Spectrum, 2}, {Dim::DSpacing, 3}}));
  EXPECT_TRUE(equals(data.values<double>(), {1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(data.unit(), units::counts);
  ASSERT_EQ(dspacing["counts"].attrs()[Dim("position")],
            tof.coords()[Dim("position")]);

  ASSERT_FALSE(dspacing.coords().contains(Dim("position")));
  ASSERT_EQ(dspacing.coords()[Dim("source-position")],
            tof.coords()[Dim("source-position")]);
  ASSERT_EQ(dspacing.coords()[Dim("sample-position")],
            tof.coords()[Dim("sample-position")]);
}

TEST_P(ConvertTest, DSpacing_to_Tof) {
  /* Assuming the Tof_to_DSpacing test is correct and passing we can test the
   * inverse conversion by simply comparing a round trip conversion with the
   * original data. */

  const Dataset tof_original = GetParam();
  const auto dspacing = convert(tof_original, Dim::Tof, Dim::DSpacing);
  const auto tof = convert(dspacing, Dim::DSpacing, Dim::Tof);

  ASSERT_TRUE(tof.contains("counts"));
  /* Broadcasting is needed as conversion introduces the dependance on
   * Dim::Spectrum */
  const auto expected_tofs =
      broadcast(tof_original.coords()[Dim::Tof], tof.coords()[Dim::Tof].dims());
  EXPECT_TRUE(equals(tof.coords()[Dim::Tof].values<double>(),
                     expected_tofs.values<double>(), 1e-12));

  ASSERT_EQ(tof.coords()[Dim("position")],
            tof_original.coords()[Dim("position")]);
  ASSERT_EQ(tof.coords()[Dim("source-position")],
            tof_original.coords()[Dim("source-position")]);
  ASSERT_EQ(tof.coords()[Dim("sample-position")],
            tof_original.coords()[Dim("sample-position")]);
}

TEST_P(ConvertTest, Tof_to_Wavelength) {
  Dataset tof = GetParam();

  auto wavelength = convert(tof, Dim::Tof, Dim::Wavelength);

  ASSERT_FALSE(wavelength.coords().contains(Dim::Tof));
  ASSERT_TRUE(wavelength.coords().contains(Dim::Wavelength));

  const auto &coord = wavelength.coords()[Dim::Wavelength];

  ASSERT_TRUE(wavelength.contains("counts"));
  EXPECT_EQ(wavelength["counts"].dims(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::Wavelength, 3}}));
  // Due to conversion, the coordinate now also depends on Dim::Spectrum.
  ASSERT_EQ(coord.dims(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::Wavelength, 4}}));
  EXPECT_EQ(coord.unit(), units::angstrom);

  const auto values = coord.values<double>();
  // Rule of thumb (https://www.psi.ch/niag/neutron-physics):
  // v [m/s] = 3956 / \lambda [ Angstrom ]
  Variable tof_in_seconds = tof.coords()[Dim::Tof] * (1e-6 * units::one);
  const auto tofs = tof_in_seconds.values<double>();
  // Spectrum 0 is 11 m from source
  EXPECT_NEAR(values[0], 3956.0 / (11.0 / tofs[0]), values[0] * 1e-3);
  EXPECT_NEAR(values[1], 3956.0 / (11.0 / tofs[1]), values[1] * 1e-3);
  EXPECT_NEAR(values[2], 3956.0 / (11.0 / tofs[2]), values[2] * 1e-3);
  EXPECT_NEAR(values[3], 3956.0 / (11.0 / tofs[3]), values[3] * 1e-3);
  // Spectrum 1
  const double L = 10.0 + sqrt(1.0 * 1.0 + 0.1 * 0.1);
  EXPECT_NEAR(values[4], 3956.0 / (L / tofs[0]), values[4] * 1e-3);
  EXPECT_NEAR(values[5], 3956.0 / (L / tofs[1]), values[5] * 1e-3);
  EXPECT_NEAR(values[6], 3956.0 / (L / tofs[2]), values[6] * 1e-3);
  EXPECT_NEAR(values[7], 3956.0 / (L / tofs[3]), values[7] * 1e-3);

  ASSERT_TRUE(wavelength.contains("counts"));
  const auto &data = wavelength["counts"];
  ASSERT_EQ(data.dims(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::Wavelength, 3}}));
  EXPECT_TRUE(equals(data.values<double>(), {1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(data.unit(), units::counts);

  for (const auto &name : {"position", "source-position", "sample-position"})
    ASSERT_EQ(wavelength.coords()[Dim(name)], tof.coords()[Dim(name)]);
}

TEST_P(ConvertTest, Wavelength_to_Tof) {
  // Assuming the Tof_to_Wavelength test is correct and passing we can test the
  // inverse conversion by simply comparing a round trip conversion with the
  // original data.

  const Dataset tof_original = GetParam();
  const auto wavelength = convert(tof_original, Dim::Tof, Dim::Wavelength);
  const auto tof = convert(wavelength, Dim::Wavelength, Dim::Tof);

  ASSERT_TRUE(tof.contains("counts"));
  // Broadcasting is needed as conversion introduces the dependance on
  // Dim::Spectrum
  EXPECT_EQ(tof.coords()[Dim::Tof], broadcast(tof_original.coords()[Dim::Tof],
                                              tof.coords()[Dim::Tof].dims()));

  ASSERT_EQ(tof.coords()[Dim("position")],
            tof_original.coords()[Dim("position")]);
  ASSERT_EQ(tof.coords()[Dim("source-position")],
            tof_original.coords()[Dim("source-position")]);
  ASSERT_EQ(tof.coords()[Dim("sample-position")],
            tof_original.coords()[Dim("sample-position")]);
}

TEST_P(ConvertTest, Tof_to_Energy_Elastic) {
  Dataset tof = GetParam();

  auto energy = convert(tof, Dim::Tof, Dim::Energy);

  ASSERT_FALSE(energy.coords().contains(Dim::Tof));
  ASSERT_TRUE(energy.coords().contains(Dim::Energy));

  const auto &coord = energy.coords()[Dim::Energy];

  constexpr auto joule_to_mev = 6.241509125883257e21;
  constexpr auto neutron_mass = 1.674927471e-27;
  /* e [J] = 1/2 m(n) [kg] (l [m] / tof [s])^2 */

  // Spectrum 1
  // sin(2 theta) = 0.1/(L-10)
  const double L = 10.0 + sqrt(1.0 * 1.0 + 0.1 * 0.1);

  ASSERT_TRUE(energy.contains("counts"));
  EXPECT_EQ(energy["counts"].dims(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::Energy, 3}}));
  // Due to conversion, the coordinate now also depends on Dim::Spectrum.
  ASSERT_EQ(coord.dims(), Dimensions({{Dim::Spectrum, 2}, {Dim::Energy, 4}}));
  EXPECT_EQ(coord.unit(), units::meV);

  const auto values = coord.values<double>();
  Variable tof_in_seconds = tof.coords()[Dim::Tof] * (1e-6 * units::one);
  const auto tofs = tof_in_seconds.values<double>();

  // Spectrum 0 is 11 m from source
  EXPECT_NEAR(values[0],
              joule_to_mev * 0.5 * neutron_mass * std::pow(11 / tofs[0], 2),
              values[0] * 1e-3);
  EXPECT_NEAR(values[1],
              joule_to_mev * 0.5 * neutron_mass * std::pow(11 / tofs[1], 2),
              values[1] * 1e-3);
  EXPECT_NEAR(values[2],
              joule_to_mev * 0.5 * neutron_mass * std::pow(11 / tofs[2], 2),
              values[2] * 1e-3);
  EXPECT_NEAR(values[3],
              joule_to_mev * 0.5 * neutron_mass * std::pow(11 / tofs[3], 2),
              values[3] * 1e-3);

  // Spectrum 1
  EXPECT_NEAR(values[4],
              joule_to_mev * 0.5 * neutron_mass * std::pow(L / tofs[0], 2),
              values[4] * 1e-3);
  EXPECT_NEAR(values[5],
              joule_to_mev * 0.5 * neutron_mass * std::pow(L / tofs[1], 2),
              values[5] * 1e-3);
  EXPECT_NEAR(values[6],
              joule_to_mev * 0.5 * neutron_mass * std::pow(L / tofs[2], 2),
              values[6] * 1e-3);
  EXPECT_NEAR(values[7],
              joule_to_mev * 0.5 * neutron_mass * std::pow(L / tofs[3], 2),
              values[7] * 1e-3);

  ASSERT_TRUE(energy.contains("counts"));
  const auto &data = energy["counts"];
  ASSERT_EQ(data.dims(), Dimensions({{Dim::Spectrum, 2}, {Dim::Energy, 3}}));
  EXPECT_TRUE(equals(data.values<double>(), {1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(data.unit(), units::counts);

  for (const auto &name : {"position", "source-position", "sample-position"})
    ASSERT_EQ(energy.coords()[Dim(name)], tof.coords()[Dim(name)]);
}

TEST_P(ConvertTest, Tof_to_Energy_Elastic_fails_if_inelastic_params_present) {
  // Note these conversion fail only because they are not implemented. It should
  // definitely be possible to support this.
  Dataset tof = GetParam();
  EXPECT_NO_THROW_DISCARD(convert(tof, Dim::Tof, Dim::Energy));
  tof.coords().set(Dim::IncidentEnergy, 2.1 * units::meV);
  EXPECT_THROW_DISCARD(convert(tof, Dim::Tof, Dim::Energy), std::runtime_error);
  tof.coords().erase(Dim::IncidentEnergy);
  EXPECT_NO_THROW_DISCARD(convert(tof, Dim::Tof, Dim::Energy));
  tof.coords().set(Dim::FinalEnergy, 2.1 * units::meV);
  EXPECT_THROW_DISCARD(convert(tof, Dim::Tof, Dim::Energy), std::runtime_error);
}

TEST_P(ConvertTest, Energy_to_Tof_Elastic) {
  /* Assuming the Tof_to_Energy_Elastic test is correct and passing we can test
   * the inverse conversion by simply comparing a round trip conversion with
   * the original data. */

  const Dataset tof_original = GetParam();
  const auto energy = convert(tof_original, Dim::Tof, Dim::Energy);
  const auto tof = convert(energy, Dim::Energy, Dim::Tof);

  ASSERT_TRUE(tof.contains("counts"));
  /* Broadcasting is needed as conversion introduces the dependance on
   * Dim::Spectrum */
  const auto expected =
      broadcast(tof_original.coords()[Dim::Tof], tof.coords()[Dim::Tof].dims());
  EXPECT_TRUE(equals(tof.coords()[Dim::Tof].values<double>(),
                     expected.values<double>(), 1e-12));

  ASSERT_EQ(tof.coords()[Dim("position")],
            tof_original.coords()[Dim("position")]);
  ASSERT_EQ(tof.coords()[Dim("source-position")],
            tof_original.coords()[Dim("source-position")]);
  ASSERT_EQ(tof.coords()[Dim("sample-position")],
            tof_original.coords()[Dim("sample-position")]);
}

TEST_P(ConvertTest, Tof_to_EnergyTransfer) {
  Dataset tof = GetParam();
  EXPECT_THROW_DISCARD(convert(tof, Dim::Tof, Dim::EnergyTransfer),
                       std::runtime_error);
  tof.coords().set(Dim::IncidentEnergy, 35.0 * units::meV);
  const auto direct = convert(tof, Dim::Tof, Dim::EnergyTransfer);
  auto tof_direct = convert(direct, Dim::EnergyTransfer, Dim::Tof);
  ASSERT_TRUE(all(is_approx(tof_direct.coords()[Dim::Tof],
                            tof.coords()[Dim::Tof], 1e-9 * units::us))
                  .value<bool>());
  tof_direct.coords().set(Dim::Tof, tof.coords()[Dim::Tof]);
  EXPECT_EQ(tof_direct, tof);

  tof.coords().set(Dim::FinalEnergy, 35.0 * units::meV);
  EXPECT_THROW_DISCARD(convert(tof, Dim::Tof, Dim::EnergyTransfer),
                       std::runtime_error);
  tof.coords().erase(Dim::IncidentEnergy);
  const auto indirect = convert(tof, Dim::Tof, Dim::EnergyTransfer);
  auto tof_indirect = convert(indirect, Dim::EnergyTransfer, Dim::Tof);
  ASSERT_TRUE(all(is_approx(tof_indirect.coords()[Dim::Tof],
                            tof.coords()[Dim::Tof], 1e-9 * units::us))
                  .value<bool>());
  tof_indirect.coords().set(Dim::Tof, tof.coords()[Dim::Tof]);
  EXPECT_EQ(tof_indirect, tof);

  EXPECT_NE(direct.coords()[Dim::EnergyTransfer],
            indirect.coords()[Dim::EnergyTransfer]);
}

TEST_P(ConvertTest, convert_with_factor_type_promotion) {
  Dataset tof = GetParam();
  tof.setCoord(Dim::Tof,
               makeVariable<float>(Dims{Dim::Tof}, Shape{4}, units::us,
                                   Values{4000, 5000, 6100, 7300}));
  for (auto &&d : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
    auto res = convert(tof, Dim::Tof, d);
    EXPECT_EQ(res.coords()[d].dtype(), core::dtype<float>);

    res = convert(res, d, Dim::Tof);
    EXPECT_EQ(res.coords()[Dim::Tof].dtype(), core::dtype<float>);
  }
}

TEST(ConvertBucketsTest, events_converted) {
  Dataset tof = makeTofDataset();
  // Standard dense coord for comparison purposes. The final 0 is a dummy.
  const auto coord = makeVariable<double>(
      Dims{Dim::Spectrum, Dim::Tof}, Shape{2, 4}, units::us,
      Values{1000, 3000, 2000, 4000, 5000, 6000, 3000, 0});
  tof.coords().set(Dim::Tof, coord);
  tof.setData("bucketed", makeTofBucketedEvents());
  for (auto &&d : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
    auto res = convert(tof, Dim::Tof, d);
    auto values = res["bucketed"].values<bucket<DataArray>>();
    Variable expected(
        res.coords()[d].slice({Dim::Spectrum, 0}).slice({d, 0, 4}));
    expected.rename(d, Dim::Event);
    EXPECT_FALSE(values[0].coords().contains(Dim::Tof));
    EXPECT_TRUE(values[0].coords().contains(d));
    EXPECT_EQ(values[0].coords()[d], expected);
    expected =
        Variable(res.coords()[d].slice({Dim::Spectrum, 1}).slice({d, 0, 3}));
    expected.rename(d, Dim::Event);
    EXPECT_FALSE(values[1].coords().contains(Dim::Tof));
    EXPECT_TRUE(values[1].coords().contains(d));
    EXPECT_EQ(values[1].coords()[d], expected);
  }
}
