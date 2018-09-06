/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "dataset_view.h"

Dimensions DimensionHelper<Coord::SpectrumPosition>::get(
    const Dataset &dataset, const std::set<Dimension> &fixedDimensions) {
  return dataset.dimensions<Coord::DetectorGrouping>();
}

Dimensions
DimensionHelper<Data::StdDev>::get(const Dataset &dataset,
                                   const std::set<Dimension> &fixedDimensions) {
  return dataset.dimensions<Data::Variance>();
}
