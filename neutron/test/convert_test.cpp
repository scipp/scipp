// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/counts.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/unaligned.h"
#include "scipp/neutron/convert.h"
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

Dataset makeTofDatasetEvents() {
  Dataset tof = makeBeamline();

  auto events = makeVariable<event_list<double>>(Dims{Dim::Spectrum}, Shape{2});
  events.setUnit(units::us);
  auto eventLists = events.values<event_list<double>>();
  eventLists[0] = {1000, 3000, 2000, 4000};
  eventLists[1] = {5000, 6000, 3000};
  tof.setCoord(Dim::Tof, events);
  tof.setCoord(Dim("aux"), events);

  tof.setData("events",
              makeVariable<double>(Dims{Dim::Spectrum}, Shape{2}, units::counts,
                                   Values{1, 1}, Variances{1, 1}));

  return tof;
}

Variable makeCountDensityData(const units::Unit &unit) {
  return makeVariable<double>(Dims{Dim::Spectrum, Dim::Tof}, Shape{2, 3},
                              units::counts / unit, Values{1, 2, 3, 4, 5, 6});
}

class ConvertTest : public testing::TestWithParam<Dataset> {};

INSTANTIATE_TEST_SUITE_P(SingleEntryDataset, ConvertTest,
                         testing::Values(makeTofDataset(),
                                         makeTofDatasetEvents()));

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

  ASSERT_TRUE(dspacing.contains("counts") || dspacing.contains("events"));
  if (dspacing.contains("counts")) {
    EXPECT_EQ(dspacing["counts"].dims(),
              Dimensions({{Dim::Spectrum, 2}, {Dim::DSpacing, 3}}));
    // Due to conversion, the coordinate now also depends on Dim::Spectrum.
    ASSERT_EQ(coord.dims(),
              Dimensions({{Dim::Spectrum, 2}, {Dim::DSpacing, 4}}));
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
    ASSERT_EQ(data.dims(),
              Dimensions({{Dim::Spectrum, 2}, {Dim::DSpacing, 3}}));
    EXPECT_TRUE(equals(data.values<double>(), {1, 2, 3, 4, 5, 6}));
    EXPECT_EQ(data.unit(), units::counts);
  } else {
    ASSERT_TRUE(dspacing.contains("events"));
    const auto &events = dspacing["events"];
    ASSERT_EQ(events.dims(), Dimensions(Dim::Spectrum, 2));
    const auto &tof0 =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>()[0];
    const auto &d0 =
        events.coords()[Dim::DSpacing].values<event_list<double>>()[0];
    ASSERT_EQ(scipp::size(d0), 4);
    EXPECT_NEAR(d0[0], 3956.0 / (1e6 * 11.0 / tof0[0]) / sqrt(2.0),
                d0[0] * 1e-3);
    EXPECT_NEAR(d0[1], 3956.0 / (1e6 * 11.0 / tof0[1]) / sqrt(2.0),
                d0[1] * 1e-3);
    EXPECT_NEAR(d0[2], 3956.0 / (1e6 * 11.0 / tof0[2]) / sqrt(2.0),
                d0[2] * 1e-3);
    EXPECT_NEAR(d0[3], 3956.0 / (1e6 * 11.0 / tof0[3]) / sqrt(2.0),
                d0[3] * 1e-3);
    const auto &tof1 =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>()[1];
    const auto &d1 =
        events.coords()[Dim::DSpacing].values<event_list<double>>()[1];
    ASSERT_EQ(scipp::size(d1), 3);
    EXPECT_NEAR(d1[0], 3956.0 / (1e6 * 11.0 / tof1[0]) * lambda_to_d,
                d1[0] * 1e-3);
    EXPECT_NEAR(d1[1], 3956.0 / (1e6 * 11.0 / tof1[1]) * lambda_to_d,
                d1[1] * 1e-3);
    EXPECT_NEAR(d1[2], 3956.0 / (1e6 * 11.0 / tof1[2]) * lambda_to_d,
                d1[2] * 1e-3);
  }

  ASSERT_EQ(dspacing.attrs()["position"], tof.coords()[Dim("position")]);
  ASSERT_EQ(dspacing.coords()[Dim("source-position")],
            tof.coords()[Dim("source-position")]);
  ASSERT_EQ(dspacing.coords()[Dim("sample-position")],
            tof.coords()[Dim("sample-position")]);
}

TEST(Convert, converts_events_labels) {
  // label "conversion" is name change of dim
  Dataset tof = makeTofDatasetEvents();
  Dataset dspacing = convert(tof, Dim::Tof, Dim::DSpacing);
  Dimensions expected(Dim::Spectrum, 2);
  EXPECT_EQ(dspacing["events"].coords()[Dim::DSpacing].dims(), expected);
  EXPECT_EQ(dspacing["events"].coords()[Dim("aux")].dims(), expected);
}

TEST_P(ConvertTest, DSpacing_to_Tof) {
  /* Assuming the Tof_to_DSpacing test is correct and passing we can test the
   * inverse conversion by simply comparing a round trip conversion with the
   * original data. */

  const Dataset tof_original = GetParam();
  const auto dspacing = convert(tof_original, Dim::Tof, Dim::DSpacing);
  const auto tof = convert(dspacing, Dim::DSpacing, Dim::Tof);

  ASSERT_TRUE(tof.contains("counts") || tof.contains("events"));
  if (tof.contains("counts")) {
    /* Broadcasting is needed as conversion introduces the dependance on
     * Dim::Spectrum */
    const auto expected_tofs = broadcast(tof_original.coords()[Dim::Tof],
                                         tof.coords()[Dim::Tof].dims());
    EXPECT_TRUE(equals(tof.coords()[Dim::Tof].values<double>(),
                       expected_tofs.values<double>(), 1e-12));
  } else {
    ASSERT_TRUE(tof.contains("events"));
    const auto events =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>();
    const auto events_original =
        tof_original["events"].coords()[Dim::Tof].values<event_list<double>>();
    EXPECT_TRUE(equals(events[0], events_original[0], 1e-15));
    EXPECT_TRUE(equals(events[1], events_original[1], 1e-12));
  }

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

  ASSERT_TRUE(wavelength.contains("counts") || wavelength.contains("events"));
  if (wavelength.contains("counts")) {
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
  } else {
    ASSERT_TRUE(wavelength.contains("events"));
    const auto &events = wavelength["events"];
    ASSERT_EQ(events.dims(), Dimensions(Dim::Spectrum, 2));
    const auto &tof0 =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>()[0];
    const auto &d0 =
        events.coords()[Dim::Wavelength].values<event_list<double>>()[0];
    ASSERT_EQ(scipp::size(d0), 4);
    EXPECT_NEAR(d0[0], 3956.0 / (1e6 * 11.0 / tof0[0]), d0[0] * 1e-3);
    EXPECT_NEAR(d0[1], 3956.0 / (1e6 * 11.0 / tof0[1]), d0[1] * 1e-3);
    EXPECT_NEAR(d0[2], 3956.0 / (1e6 * 11.0 / tof0[2]), d0[2] * 1e-3);
    EXPECT_NEAR(d0[3], 3956.0 / (1e6 * 11.0 / tof0[3]), d0[3] * 1e-3);
    const auto &tof1 =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>()[1];
    const auto &d1 =
        events.coords()[Dim::Wavelength].values<event_list<double>>()[1];
    ASSERT_EQ(scipp::size(d1), 3);
    EXPECT_NEAR(d1[0], 3956.0 / (1e6 * 11.0 / tof1[0]), d1[0] * 1e-3);
    EXPECT_NEAR(d1[1], 3956.0 / (1e6 * 11.0 / tof1[1]), d1[1] * 1e-3);
    EXPECT_NEAR(d1[2], 3956.0 / (1e6 * 11.0 / tof1[2]), d1[2] * 1e-3);
  }

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

  ASSERT_TRUE(tof.contains("counts") || tof.contains("events"));
  if (tof.contains("counts")) {
    // Broadcasting is needed as conversion introduces the dependance on
    // Dim::Spectrum
    EXPECT_EQ(tof.coords()[Dim::Tof], broadcast(tof_original.coords()[Dim::Tof],
                                                tof.coords()[Dim::Tof].dims()));
  } else {
    ASSERT_TRUE(tof.contains("events"));
    const auto events =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>();
    const auto events_original =
        tof_original["events"].coords()[Dim::Tof].values<event_list<double>>();
    EXPECT_TRUE(equals(events[0], events_original[0], 1e-15));
    EXPECT_TRUE(equals(events[1], events_original[1], 1e-12));
  }

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

  ASSERT_TRUE(energy.contains("counts") || energy.contains("events"));
  if (energy.contains("counts")) {
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
  } else {
    ASSERT_TRUE(energy.contains("events"));
    const auto &events = energy["events"];
    ASSERT_EQ(events.dims(), Dimensions(Dim::Spectrum, 2));
    const auto &tof0 =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>()[0];
    const auto &e0 =
        events.coords()[Dim::Energy].values<event_list<double>>()[0];
    ASSERT_EQ(scipp::size(e0), 4);
    EXPECT_NEAR(e0[0],
                joule_to_mev * 0.5 * neutron_mass *
                    std::pow(1e6 * 11 / tof0[0], 2),
                e0[0] * 1e-3);
    EXPECT_NEAR(e0[1],
                joule_to_mev * 0.5 * neutron_mass *
                    std::pow(1e6 * 11 / tof0[1], 2),
                e0[1] * 1e-3);
    EXPECT_NEAR(e0[2],
                joule_to_mev * 0.5 * neutron_mass *
                    std::pow(1e6 * 11 / tof0[2], 2),
                e0[2] * 1e-3);
    EXPECT_NEAR(e0[3],
                joule_to_mev * 0.5 * neutron_mass *
                    std::pow(1e6 * 11 / tof0[3], 2),
                e0[3] * 1e-3);
    const auto &tof1 =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>()[1];
    const auto &e1 =
        events.coords()[Dim::Energy].values<event_list<double>>()[1];
    ASSERT_EQ(scipp::size(e1), 3);
    EXPECT_NEAR(e1[0],
                joule_to_mev * 0.5 * neutron_mass *
                    std::pow(1e6 * L / tof1[0], 2),
                e1[0] * 1e-3);
    EXPECT_NEAR(e1[1],
                joule_to_mev * 0.5 * neutron_mass *
                    std::pow(1e6 * L / tof1[1], 2),
                e1[1] * 1e-3);
    EXPECT_NEAR(e1[2],
                joule_to_mev * 0.5 * neutron_mass *
                    std::pow(1e6 * L / tof1[2], 2),
                e1[2] * 1e-3);
  }

  for (const auto &name : {"position", "source-position", "sample-position"})
    ASSERT_EQ(energy.coords()[Dim(name)], tof.coords()[Dim(name)]);
}

TEST_P(ConvertTest, Energy_to_Tof_Elastic) {
  /* Assuming the Tof_to_Energy_Elastic test is correct and passing we can test
   * the inverse conversion by simply comparing a round trip conversion with
   * the original data. */

  const Dataset tof_original = GetParam();
  const auto energy = convert(tof_original, Dim::Tof, Dim::Energy);
  const auto tof = convert(energy, Dim::Energy, Dim::Tof);

  ASSERT_TRUE(tof.contains("counts") || tof.contains("events"));
  if (tof.contains("counts")) {
    /* Broadcasting is needed as conversion introduces the dependance on
     * Dim::Spectrum */
    const auto expected = broadcast(tof_original.coords()[Dim::Tof],
                                    tof.coords()[Dim::Tof].dims());
    EXPECT_TRUE(equals(tof.coords()[Dim::Tof].values<double>(),
                       expected.values<double>(), 1e-12));
  } else {
    ASSERT_TRUE(tof.contains("events"));
    const auto events =
        tof["events"].coords()[Dim::Tof].values<event_list<double>>();
    const auto events_original =
        tof_original["events"].coords()[Dim::Tof].values<event_list<double>>();
    EXPECT_TRUE(equals(events[0], events_original[0], 1e-15));
    EXPECT_TRUE(equals(events[1], events_original[1], 1e-15));
  }

  ASSERT_EQ(tof.coords()[Dim("position")],
            tof_original.coords()[Dim("position")]);
  ASSERT_EQ(tof.coords()[Dim("source-position")],
            tof_original.coords()[Dim("source-position")]);
  ASSERT_EQ(tof.coords()[Dim("sample-position")],
            tof_original.coords()[Dim("sample-position")]);
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

TEST(ConvertRealignedTest, default) {
  const Dataset events = makeTofDatasetEvents();
  const auto tof_edges = makeVariable<double>(
      Dims{Dim::Tof}, Shape{4}, units::us, Values{0, 1500, 5500, 6500});
  auto realigned = dataset::unaligned::realign(events, {{Dim::Tof, tof_edges}});
  // Cannot use this testing approach for Dim::Energy since it leads to
  // decreasing coord which prevents histograming.
  for (const auto &dim : {Dim::DSpacing, Dim::Wavelength})
    EXPECT_EQ(histogram(convert(realigned, Dim::Tof, dim)),
              convert(histogram(events, tof_edges), Dim::Tof, dim));
}

TEST(ConvertRealignedTest, realign_linear) {
  const Dataset events = makeTofDatasetEvents();
  const auto tof_edges = makeVariable<double>(
      Dims{Dim::Tof}, Shape{4}, units::us, Values{500, 1500, 5500, 6500});
  auto realigned = dataset::unaligned::realign(events, {{Dim::Tof, tof_edges}});
  for (const auto &dim : {Dim::DSpacing, Dim::Wavelength, Dim::Energy}) {
    // Comparing to case without realignment enabled
    const auto reference = convert(realigned, Dim::Tof, dim);
    auto out = convert(realigned, Dim::Tof, dim, ConvertRealign::Linear);
    const auto coord = out.coords()[dim];
    EXPECT_EQ(min(coord), min(reference.coords()[dim]));
    EXPECT_EQ(max(coord), max(reference.coords()[dim]));
    EXPECT_EQ(coord.dims(), Dimensions(dim, 4));
    out.coords().erase(dim);
    out.coords().set(dim, reference.coords()[dim]);
    EXPECT_EQ(out, reference);
  }
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
