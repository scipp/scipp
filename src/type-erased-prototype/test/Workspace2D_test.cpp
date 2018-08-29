#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset_index.h"
#include "dataset_view.h"

TEST(Workspace2D, basics) {
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
  auto delta = slice(combined, Dimension::Polarization, spin["spin-up"]) +
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
