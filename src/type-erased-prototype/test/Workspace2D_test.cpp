#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset_index.h"
#include "dataset_view.h"

TEST(Workspace2D, multi_dimensional_merging_and_slicing) {
  Dataset d;

  // Scalar metadata using existing Mantid classes:
  // d.insert<Coord::Sample>({}, API::Sample{});
  // d.insert<Coord::Run>({}, API::Run{});

  // Instrument
  // Scalar part of instrument, e.g., something like this:
  // d.insert<Coord::Instrument>({}, Beamline::ComponentInfo{});
  d.insert<Coord::DetectorId>({Dimension::Detector, 4},
                              {1001, 1002, 1003, 1004});
  d.insert<Coord::DetectorPosition>({Dimension::Detector, 4},
                                    {1.0, 2.0, 4.0, 8.0});

  // Spectrum to detector mapping and spectrum numbers.
  std::vector<std::vector<gsl::index>> grouping = {{0, 2}, {1}, {}};
  d.insert<Coord::DetectorGrouping>({Dimension::Spectrum, 3}, grouping);
  d.insert<Coord::SpectrumNumber>({Dimension::Spectrum, 3}, {1, 2, 3});

  // "X" axis (shared for all spectra).
  d.insert<Coord::Tof>({Dimension::Tof, 1000}, 1000);
  // Y
  d.insert<Data::Value>(
      "sample", Dimensions({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}}),
      3 * 1000);
  // E
  d.insert<Data::Variance>(
      "sample", Dimensions({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}}),
      3 * 1000);

  // Monitors
  d.insert<Coord::MonitorTof>({Dimension::MonitorTof, 222}, 222);
  d.insert<Data::Value>("monitor", Dimensions({{Dimension::MonitorTof, 222},
                                               {Dimension::Monitor, 2}}),
                        2 * 222);
  d.insert<Data::Variance>("monitor", Dimensions({{Dimension::MonitorTof, 222},
                                                  {Dimension::Monitor, 2}}),
                           2 * 222);

  auto spinUp(d);
  auto spinDown(d);

  // Aka WorkspaceSingleValue
  Dataset offset;
  offset.insert<Data::Value>("sample", {}, {1.0});
  offset.insert<Data::Variance>("sample", {}, {0.1});
  // Note the use of name "sample" such that offset affects sample, not
  // other `Data` variables such as monitors.
  spinDown += offset;

  // Combine data for spin-up and spin-down in same dataset, polarization is an
  // extra dimension.
  auto combined = concatenate(Dimension::Polarization, spinUp, spinDown);
  combined.insert<Coord::Polarization>(
      {Dimension::Polarization, 2},
      std::vector<std::string>{"spin-up", "spin-down"});

  // Do a temperature scan, adding a new temperature dimension to the dataset.
  combined.insert<Coord::Temperature>({}, {300.0});
  combined.get<Data::Value>("sample")[0] = exp(-0.001 * 300.0);
  auto dataPoint(combined);
  for (const auto temperature : {273.0, 200.0, 100.0, 10.0, 4.2}) {
    dataPoint.get<Coord::Temperature>()[0] = temperature;
    dataPoint.get<Data::Value>("sample")[0] = exp(-0.001 * temperature);
    combined = concatenate(Dimension::Temperature, combined, dataPoint);
  }

  // Compute spin difference.
  DatasetIndex<Coord::Polarization> spin(combined);
  combined.erase<Coord::Polarization>();
  auto delta = slice(combined, Dimension::Polarization, spin["spin-up"]) -
               slice(combined, Dimension::Polarization, spin["spin-down"]);

  // Extract a single Tof slice.
  delta = slice(delta, Dimension::Tof, 0);

  using PointData = DatasetView<const Coord::Temperature, const Data::Value,
                                const Data::Variance>;
  DatasetView<PointData, const Coord::SpectrumNumber> view(
      delta, "sample", {Dimension::Temperature});

  auto tempDependence =
      std::find_if(view.begin(), view.end(), [](const auto &item) {
        return item.template get<Coord::SpectrumNumber>() == 1;
      })->get<PointData>();

  for (const auto &point : tempDependence)
    printf("%lf %lf %lf\n", point.get<Coord::Temperature>(), point.value(),
           point.get<Data::Variance>());
}

TEST(Workspace2D, multiple_data) {
  Dataset d;

  d.insert<Coord::Tof>({Dimension::Tof, 1000}, 1000);

  // Sample
  d.insert<Data::Value>(
      "sample", Dimensions({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}}),
      3 * 1000);
  d.insert<Data::Variance>(
      "sample", Dimensions({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}}),
      3 * 1000);

  // Background
  d.insert<Data::Value>(
      "background",
      Dimensions({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}}), 3 * 1000);
  d.insert<Data::Variance>(
      "background",
      Dimensions({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}}), 3 * 1000);

  // Monitors
  d.insert<Coord::MonitorTof>({Dimension::MonitorTof, 222}, 222);
  d.insert<Data::Value>("monitor", Dimensions({{Dimension::MonitorTof, 222},
                                               {Dimension::Monitor, 2}}),
                        2 * 222);
  d.insert<Data::Variance>("monitor", Dimensions({{Dimension::MonitorTof, 222},
                                                  {Dimension::Monitor, 2}}),
                           2 * 222);

  d.merge(d.extract("sample") - d.extract("background"));

  EXPECT_NO_THROW(d.get<const Data::Value>("sample - background"));
  EXPECT_NO_THROW(d.get<const Data::Variance>("sample - background"));
  EXPECT_NO_THROW(d.get<const Data::Value>("monitor"));
  EXPECT_ANY_THROW(d.get<const Data::Value>("sample"));
  EXPECT_ANY_THROW(d.get<const Data::Value>("background"));
}

TEST(Workspace2D, scanning) {
  Dataset d;

  // Scalar part of instrument, e.g.:
  // d.insert<Coord::Instrument>({}, Beamline::ComponentInfo{});
  d.insert<Coord::DetectorId>({Dimension::Detector, 4},
                              {1001, 1002, 1003, 1004});
  d.insert<Coord::DetectorPosition>({Dimension::Detector, 4},
                                    {1.0, 2.0, 3.0, 4.0});

  // In the current implementation in Mantid, ComponentInfo holds a reference to
  // DetectorInfo. Now the contents of DetectorInfo are simply variables in the
  // dataset. Keeping references to the dataset does not seem to be the right
  // solution. Instead we could have a helper class dealing with movements or
  // access to positions of all components that is constructed on the fly:
  // class InstrumentView {
  //   InstrumentView(Dataset &d);
  //   void setPosition(const gsl::index i, const Eigen::Vector3d &pos) {
  //     if (i < m_dataset.dimensions().size(Dimension::Detector))
  //       m_detPos[i] = pos;
  //     else {
  //       // recursive move as implemented in Beamline::ComponentInfo
  //     }
  //   }
  // };
  auto moved(d);
  for (auto &pos : moved.get<Coord::DetectorPosition>())
    pos += 0.5;

  auto scanning = concatenate(Dimension::DetectorScan, d, moved);
  scanning.insert<Coord::TimeInterval>(
      {Dimension::DetectorScan, 2},
      {std::make_pair(0l, 10l), std::make_pair(10l, 20l)});

  // Spectrum to detector mapping and spectrum numbers. Currently this mapping
  // is purely positional. We may consider changing this to an two-part
  // (detector-index, time-index). In any case, since the mapping is based on
  // indices we need to take this into account in the implementation of
  // slicing/dicing and merging operations such that indices are updated
  // accordingly. Probably the easiest solution is to forbid shape operations on
  // Dimension::Detector and Dimension::DetectorScan if Coord::DetectorGrouping
  // is present.
  std::vector<std::vector<gsl::index>> grouping = {{0}, {2}, {4}};
  scanning.insert<Coord::DetectorGrouping>({Dimension::Spectrum, 3}, grouping);
  scanning.insert<Coord::SpectrumNumber>({Dimension::Spectrum, 3}, {1, 2, 3});

  DatasetView<Coord::SpectrumPosition> view(scanning);
  ASSERT_EQ(view.size(), 3);
  auto it = view.begin();
  EXPECT_EQ(it++->get<Coord::SpectrumPosition>(), 1.0);
  EXPECT_EQ(it++->get<Coord::SpectrumPosition>(), 3.0);
  EXPECT_EQ(it++->get<Coord::SpectrumPosition>(), 1.5);
}
