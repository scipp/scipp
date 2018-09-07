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

template <class... Ts>
Dimensions DatasetViewImpl<Ts...>::relevantDimensions(
    const Dataset &dataset,
    boost::container::small_vector<Dimensions, 4> variableDimensions,
    const std::set<Dimension> &fixedDimensions) const {
  // The dimensions for the variables may be longer by one if the variable is
  // an edge variable. For iteration dimensions we require the dimensions
  // without the extended length. The original variableDimensions is kept
  // (note pass by value) since the extended length is required to compute the
  // correct offset into the variable.
  std::vector<bool> is_bins{detail::is_bins_t<Ts>::value...};
  for (gsl::index i = 0; i < sizeof...(Ts); ++i) {
    auto &dims = variableDimensions[i];
    if (is_bins[i]) {
      const auto &actual = dataset.dimensions();
      for (auto &dim : dims)
        dims.resize(dim.first, actual.size(dim.first));
    }
  }

  auto largest =
      *std::max_element(variableDimensions.begin(), variableDimensions.end(),
                        [](const Dimensions &a, const Dimensions &b) {
                          return a.count() < b.count();
                        });
  for (const auto dim : fixedDimensions)
    if (largest.contains(dim))
      largest.erase(dim);

  std::vector<bool> is_const{detail::is_const<Ts>::value...};
  for (gsl::index i = 0; i < sizeof...(Ts); ++i) {
    auto dims = variableDimensions[i];
    for (const auto dim : fixedDimensions)
      if (dims.contains(dim))
        dims.erase(dim);
    // Largest must contain all other dimensions.
    if (!largest.contains(dims))
      throw std::runtime_error(
          "Variables requested for iteration do not span a joint space. In "
          "case one of the variables represents bin edges direct joint "
          "iteration is not possible. Use the Bin<> wrapper to iterate over "
          "bins defined by edges instead.");
    // Must either be identical or access must be read-only.
    if (!((largest == dims) || is_const[i]))
      throw std::runtime_error("Variables requested for iteration have "
                               "different dimensions");
  }
  return largest;
}

#define INSTANTIATE(...)                                                       \
  template Dimensions DatasetViewImpl<__VA_ARGS__>::relevantDimensions(        \
      const Dataset &, boost::container::small_vector<Dimensions, 4>,          \
      const std::set<Dimension> &) const;

// For very special cases we should probably provide a header that includes the
// definition, so we do not need to instantiate everything directly, but just
// the most common cases.
INSTANTIATE(Bin<Coord::Tof>)
INSTANTIATE(Bin<Coord::Tof> const)
INSTANTIATE(Bin<Coord::Tof>, Data::Int)
INSTANTIATE(Bin<Coord::Tof>, Data::Value, Data::Variance)
INSTANTIATE(Bin<Coord::X>)
INSTANTIATE(Bin<Coord::Y>)
INSTANTIATE(Coord::Mask const, DatasetViewImpl<Data::Value>)
INSTANTIATE(Coord::Mask const, Data::Value, Data::Variance)
INSTANTIATE(Coord::SpectrumNumber const,
            DatasetViewImpl<Coord::Temperature const, Data::Value const,
                            Data::Variance const>)
INSTANTIATE(Coord::SpectrumNumber,
            DatasetViewImpl<Bin<Coord::Tof>, Data::Value, Data::Variance>)
INSTANTIATE(Coord::SpectrumPosition)
INSTANTIATE(Coord::Temperature const, Data::Value const, Data::Variance const)
INSTANTIATE(Coord::Tof)
INSTANTIATE(Coord::Tof, Data::Int)
INSTANTIATE(Coord::X const)
INSTANTIATE(Coord::X const, Coord::Y)
INSTANTIATE(Coord::X const, Coord::Y const)
INSTANTIATE(Coord::X const, Data::Value const)
INSTANTIATE(Coord::X, Coord::Y)
INSTANTIATE(Coord::X, Coord::Y const)
INSTANTIATE(Coord::X, DatasetViewImpl<Coord::Y>)
INSTANTIATE(Coord::X, DatasetViewImpl<Coord::Y const, Coord::Z>)
INSTANTIATE(Coord::X, DatasetViewImpl<Coord::Y, Coord::Z>)
INSTANTIATE(Coord::X, Data::Value)
INSTANTIATE(Coord::X, Data::Value const)
INSTANTIATE(Coord::Y)
INSTANTIATE(Coord::Y const, Coord::Z)
INSTANTIATE(Coord::Y, Coord::Z)
INSTANTIATE(Data::Events const,
            DatasetViewImpl<Bin<Coord::Tof>, Data::Value, Data::Variance>)
INSTANTIATE(Data::Int)
INSTANTIATE(Data::Int const, DatasetViewImpl<Data::Value const>)
INSTANTIATE(Data::Int, DatasetViewImpl<Data::Value>)
INSTANTIATE(Data::Int, DatasetViewImpl<Data::Value, Data::Variance>)
INSTANTIATE(DatasetViewImpl<Coord::X const, Data::Value const>)
INSTANTIATE(DatasetViewImpl<Coord::X, Data::Value>)
INSTANTIATE(DatasetViewImpl<Coord::X, Data::Value const>)
INSTANTIATE(DatasetViewImpl<Data::Value const>)
INSTANTIATE(Data::StdDev)
INSTANTIATE(Data::Value)
INSTANTIATE(Data::Value const)
INSTANTIATE(Data::Value const, Data::String)
INSTANTIATE(Data::Value, Data::Int)
INSTANTIATE(Data::Value, Data::Int const)
INSTANTIATE(Data::Value, Data::Variance)
INSTANTIATE(Data::Value, Data::Variance const)
INSTANTIATE(Data::Variance, Data::Int)
