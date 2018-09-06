/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
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
  Vector<std::vector<gsl::index>> grouping = {{0, 2}, {1}, {}};
  d.insert<Coord::DetectorGrouping>({Dimension::Spectrum, 3}, grouping);
  d.insert<Coord::SpectrumNumber>({Dimension::Spectrum, 3}, {1, 2, 3});

  // "X" axis (shared for all spectra).
  d.insert<Coord::Tof>({Dimension::Tof, 1000}, 1000);
  Dimensions dims({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}});
  // Y
  d.insert<Data::Value>("sample", dims, dims.volume());
  // E
  d.insert<Data::Variance>("sample", dims, dims.volume());

  // Monitors
  dims = Dimensions({{Dimension::MonitorTof, 222}, {Dimension::Monitor, 2}});
  d.insert<Coord::MonitorTof>({Dimension::MonitorTof, 222}, 222);
  d.insert<Data::Value>("monitor", dims, dims.volume());
  d.insert<Data::Variance>("monitor", dims, dims.volume());

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
      Vector<std::string>{"spin-up", "spin-down"});

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

  // Do something with the resulting point data, e.g., plot:
  // for (const auto &point : tempDependence)
  //   plotPoint(point.get<Coord::Temperature>(), point.value(),
  //             point.get<Data::Variance>());
}

TEST(Workspace2D, multiple_data) {
  Dataset d;

  d.insert<Coord::Tof>({Dimension::Tof, 1000}, 1000);

  Dimensions dims({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}});

  // Sample
  d.insert<Data::Value>("sample", dims, dims.volume());
  d.insert<Data::Variance>("sample", dims, dims.volume());

  // Background
  d.insert<Data::Value>("background", dims, dims.volume());
  d.insert<Data::Variance>("background", dims, dims.volume());

  // Monitors
  dims = Dimensions({{Dimension::MonitorTof, 222}, {Dimension::Monitor, 2}});
  d.insert<Coord::MonitorTof>({Dimension::MonitorTof, 222}, 222);
  d.insert<Data::Value>("monitor", dims, dims.volume());
  d.insert<Data::Variance>("monitor", dims, dims.volume());

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
  Vector<std::vector<gsl::index>> grouping = {{0}, {2}, {4}};
  scanning.insert<Coord::DetectorGrouping>({Dimension::Spectrum, 3}, grouping);
  scanning.insert<Coord::SpectrumNumber>({Dimension::Spectrum, 3}, {1, 2, 3});

  DatasetView<Coord::SpectrumPosition> view(scanning);
  ASSERT_EQ(view.size(), 3);
  auto it = view.begin();
  EXPECT_EQ(it++->get<Coord::SpectrumPosition>(), 1.0);
  EXPECT_EQ(it++->get<Coord::SpectrumPosition>(), 3.0);
  EXPECT_EQ(it++->get<Coord::SpectrumPosition>(), 1.5);
}

TEST(Workspace2D, masking) {
  // Solution for masking not clear, the following shows one option.

  Dataset d;

  d.insert<Coord::Tof>({Dimension::Tof, 1000}, 1000);
  Dimensions dims({{Dimension::Tof, 1000}, {Dimension::Spectrum, 3}});
  // Sample
  d.insert<Data::Value>("sample", dims, dims.volume());
  d.insert<Data::Variance>("sample", dims, dims.volume());
  // Background
  d.insert<Data::Value>("background", dims, dims.volume());
  d.insert<Data::Variance>("background", dims, dims.volume());

  // Spectra mask.
  // Can be in its own Dataset to support loading, saving, and manipulation.
  Dataset mask;
  mask.insert<Coord::Mask>({Dimension::Spectrum, 3}, Vector<char>{0, 0, 1});

  // Add mask to Dataset, not touching data.
  auto d_masked(d);
  d_masked.merge(mask);

  // Cannot add masked workspace to non-masked (handled implicitly by
  // requirement of matching coordinates).
  EXPECT_ANY_THROW(d += d_masked);
  // Adding non-masked to masked works, is this sensible behavior?
  EXPECT_NO_THROW(d_masked += d);

  mask.get<Coord::Mask>()[0] = 1;
  auto d_masked2(d);
  d_masked2.merge(mask);

  // If there are conflicting masks addition in any order fails, i.e., there is
  // no hidden magic.
  EXPECT_ANY_THROW(d_masked += d_masked2);
  EXPECT_ANY_THROW(d_masked2 += d_masked);

  // Remove mask.
  d_masked.erase<Coord::Mask>();

  // Skip processing spectrum if it is masked.
  EXPECT_FALSE(d_masked2.dimensions<Coord::Mask>().contains(Dimension::Tof));
  DatasetView<DatasetView<Data::Value>, const Coord::Mask> spectra(d_masked2,
                                                                   "sample");
  for (auto &item : spectra)
    if (!item.get<Coord::Mask>())
      for (auto &point : item.get<DatasetView<Data::Value>>())
        point.value() += 1.0;

  // Apply mask.
  DatasetView<Data::Value, Data::Variance, const Coord::Mask> view(
      d_masked2, "background");
  for (auto &item : view) {
    item.value() *= item.get<Coord::Mask>();
    item.get<Data::Variance>() *= item.get<Coord::Mask>();
  }
  // Could by simplified if we implement binary operations with mixed types
  // (such as double * int):
  // d_masked2.merge(d_masked2.extract("sample") *
  //                 d_masked2.extract<Coord::Mask>());

  // Bin mask.
  mask = Dataset();
  mask.insert<Coord::Mask>({Dimension::Tof, 1000}, 1000);
  mask.get<Coord::Mask>()[0] = 1;
  // mask has no Dimension::Spectrum so this masks the first bin of all spectra.
  d_masked.merge(mask);
  // Different bin masking for all spectra.
  mask = Dataset();
  mask.insert<Coord::Mask>(dims, dims.volume());
}
