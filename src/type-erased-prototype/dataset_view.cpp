/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "dataset_view.h"

Dimensions DimensionHelper<Data::Histogram>::get(
    const Dataset &dataset, const std::set<Dimension> &fixedDimensions) {
  auto dims = dataset.dimensions<Data::Value>();
  if (fixedDimensions.size() != 1)
    throw std::runtime_error(
        "Bad number of fixed dimensions. Only 1D histograms are supported.");
  dims.erase(*fixedDimensions.begin());
  return dims;
}

Dimensions DimensionHelper<Coord::SpectrumPosition>::get(
    const Dataset &dataset, const std::set<Dimension> &fixedDimensions) {
  return dataset.dimensions<Coord::DetectorGrouping>();
}

Dimensions
DimensionHelper<Data::StdDev>::get(const Dataset &dataset,
                                   const std::set<Dimension> &fixedDimensions) {
  return dataset.dimensions<Data::Variance>();
}
