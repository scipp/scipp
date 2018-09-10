/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "dataset_view.h"

template <class Tag> struct UnitHelper {
  static Unit get(const Dataset &dataset) { return dataset.unit<Tag>(); }

  static Unit get(const Dataset &dataset, const std::string &name) {
    if (is_coord<Tag>)
      return dataset.unit<Tag>();
    else
      return dataset.unit<Tag>(name);
  }
};

template <class Tag> struct UnitHelper<Bin<Tag>> {
  static Unit get(const Dataset &dataset) { return dataset.unit<Tag>(); }
  static Unit get(const Dataset &dataset, const std::string &name) {
    return dataset.unit<Tag>();
  }
};

template <> struct UnitHelper<Coord::SpectrumPosition> {
  static Unit get(const Dataset &dataset) {
    return dataset.unit<Coord::DetectorPosition>();
  }
  static Unit get(const Dataset &dataset, const std::string &name) {
    return dataset.unit<Coord::DetectorPosition>();
  }
};

template <> struct UnitHelper<Data::StdDev> {
  static Unit get(const Dataset &dataset) {
    return dataset.unit<Data::Variance>();
  }
  static Unit get(const Dataset &dataset, const std::string &name) {
    return dataset.unit<Data::Variance>();
  }
};

template <class... Tags> struct UnitHelper<DatasetViewImpl<Tags...>> {
  static detail::unit_t<DatasetViewImpl<Tags...>> get(const Dataset &dataset) {
    return std::make_tuple(UnitHelper<Tags>::get(dataset)...);
  }
  static detail::unit_t<DatasetViewImpl<Tags...>> get(const Dataset &dataset,
                                                      const std::string &name) {
    return std::make_tuple(UnitHelper<Tags>::get(dataset, name)...);
  }
};

template <class Tag> struct DimensionHelper {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    static_cast<void>(fixedDimensions);
    return dataset.dimensions<Tag>();
  }

  static Dimensions get(const Dataset &dataset, const std::string &name,
                        const std::set<Dimension> &fixedDimensions) {
    static_cast<void>(fixedDimensions);
    // TODO Do we need to check here if fixedDimensions are contained?
    if (is_coord<Tag>)
      return dataset.dimensions<Tag>();
    else
      return dataset.dimensions<Tag>(name);
  }
};

template <class Tag> struct DimensionHelper<Bin<Tag>> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    return dataset.dimensions<Tag>();
  }
  static Dimensions get(const Dataset &dataset, const std::string &name,
                        const std::set<Dimension> &fixedDimensions) {
    if (is_coord<Tag>)
      return dataset.dimensions<Tag>();
    else
      return dataset.dimensions<Tag>(name);
  }
};

template <> struct DimensionHelper<Coord::SpectrumPosition> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    return dataset.dimensions<Coord::DetectorGrouping>();
  }
  static Dimensions get(const Dataset &dataset, const std::string &name,
                        const std::set<Dimension> &fixedDimensions) {
    return dataset.dimensions<Coord::DetectorGrouping>();
  }
};

template <> struct DimensionHelper<Data::StdDev> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    return dataset.dimensions<Data::Variance>();
  }
  static Dimensions get(const Dataset &dataset, const std::string &name,
                        const std::set<Dimension> &fixedDimensions) {
    return dataset.dimensions<Data::Variance>();
  }
};

template <class... Tags> struct DimensionHelper<DatasetViewImpl<Tags...>> {
  static Dimensions getHelper(std::vector<Dimensions> variableDimensions,
                              const std::set<Dimension> &fixedDimensions) {
    // Remove fixed dimensions *before* finding largest --- outer iteration must
    // cover all contained non-fixed dimensions.
    for (auto &dims : variableDimensions)
      for (const auto dim : fixedDimensions)
        if (dims.contains(dim))
          dims.erase(dim);

    auto largest =
        *std::max_element(variableDimensions.begin(), variableDimensions.end(),
                          [](const Dimensions &a, const Dimensions &b) {
                            return a.count() < b.count();
                          });

    // Check that Tags have correct constness if dimensions do not match.
    // Usually this happens in `relevantDimensions` but for the nested case we
    // are returning only the largest set of dimensions so we have to do the
    // comparison here.
    std::vector<bool> is_const{
        std::is_const<detail::value_type_t<Tags>>::value...};
    for (gsl::index i = 0; i < sizeof...(Tags); ++i) {
      auto dims = variableDimensions[i];
      if (!((largest == dims) || is_const[i]))
        throw std::runtime_error("Variables requested for iteration have "
                                 "different dimensions");
    }
    return largest;
  }

  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    return getHelper({DimensionHelper<Tags>::get(dataset, fixedDimensions)...},
                     fixedDimensions);
  }

  static Dimensions get(const Dataset &dataset, const std::string &name,
                        const std::set<Dimension> &fixedDimensions) {
    return getHelper(
        {DimensionHelper<Tags>::get(dataset, name, fixedDimensions)...},
        fixedDimensions);
  }
};

template <class Tag> struct DataHelper {
  static auto get(MaybeConstDataset<Tag> &dataset,
                  const Dimensions &iterationDimensions) {
    return dataset.template get<detail::value_type_t<Tag>>();
  }
  static auto get(MaybeConstDataset<Tag> &dataset,
                  const Dimensions &iterationDimensions,
                  const std::string &name) {
    if (is_coord<Tag>)
      return dataset.template get<detail::value_type_t<Tag>>();
    else
      return dataset.template get<detail::value_type_t<Tag>>(name);
  }
};

template <class Tag> struct DataHelper<Bin<Tag>> {
  static auto get(const Dataset &dataset,
                  const Dimensions &iterationDimensions) {
    // Compute offset to next edge.
    gsl::index offset = 1;
    const auto &dims = dataset.dimensions<Tag>();
    const auto &actual = dataset.dimensions();
    for (const auto &dim : dims) {
      if (dim.second != actual.size(dim.first))
        break;
      offset *= dim.second;
    }

    return ref_type_t<Bin<Tag>>{offset,
                                dataset.get<const detail::value_type_t<Tag>>()};
  }
  static auto get(const Dataset &dataset, const Dimensions &iterationDimensions,
                  const std::string &name) {
    return get(dataset, iterationDimensions);
  }
};

template <> struct DataHelper<Coord::SpectrumPosition> {
  static auto get(const Dataset &dataset,
                  const Dimensions &iterationDimensions) {
    return ref_type_t<Coord::SpectrumPosition>(
        dataset.get<detail::value_type_t<const Coord::DetectorPosition>>(),
        dataset.get<detail::value_type_t<const Coord::DetectorGrouping>>());
  }
  static auto get(const Dataset &dataset, const Dimensions &iterationDimensions,
                  const std::string &name) {
    return ref_type_t<Coord::SpectrumPosition>(
        dataset.get<detail::value_type_t<const Coord::DetectorPosition>>(),
        dataset.get<detail::value_type_t<const Coord::DetectorGrouping>>());
  }
};

template <> struct DataHelper<Data::StdDev> {
  static auto get(const Dataset &dataset,
                  const Dimensions &iterationDimensions) {
    return DataHelper<const Data::Variance>::get(dataset, iterationDimensions);
  }
  static auto get(const Dataset &dataset, const Dimensions &iterationDimensions,
                  const std::string &name) {
    return DataHelper<const Data::Variance>::get(dataset, iterationDimensions);
  }
};

template <class... Tags> struct DataHelper<DatasetViewImpl<Tags...>> {
  static auto get(MaybeConstDataset<Tags...> &dataset,
                  const Dimensions &iterationDimensions) {
    std::set<Dimension> fixedDimensions;
    for (auto &item : iterationDimensions)
      fixedDimensions.insert(item.first);
    // For the nested case we create a DatasetView with the correct dimensions
    // and store it. It is later copied and initialized with the correct offset
    // in iterator::get.
    return ref_type_t<DatasetViewImpl<Tags...>>{
        MultiIndex(iterationDimensions,
                   {DimensionHelper<Tags>::get(dataset, {})...}),
        DatasetViewImpl<Tags...>(dataset, fixedDimensions),
        std::make_tuple(DataHelper<Tags>::get(dataset, {})...)};
  }
  static auto get(MaybeConstDataset<Tags...> &dataset,
                  const Dimensions &iterationDimensions,
                  const std::string &name) {
    std::set<Dimension> fixedDimensions;
    for (auto &item : iterationDimensions)
      fixedDimensions.insert(item.first);
    // For the nested case we create a DatasetView with the correct dimensions
    // and store it. It is later copied and initialized with the correct offset
    // in iterator::get.
    return ref_type_t<DatasetViewImpl<Tags...>>{
        MultiIndex(iterationDimensions,
                   {DimensionHelper<Tags>::get(dataset, name, {})...}),
        DatasetViewImpl<Tags...>(dataset, name, fixedDimensions),
        std::make_tuple(DataHelper<Tags>::get(dataset, {}, name)...)};
  }
};

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

template <class... Ts>
DatasetViewImpl<Ts...>::DatasetViewImpl(
    MaybeConstDataset<Ts...> &dataset, const std::string &name,
    const std::set<Dimension> &fixedDimensions)
    : m_units{UnitHelper<Ts>::get(dataset, name)...},
      m_variables(makeVariables(dataset, name, fixedDimensions)) {}
template <class... Ts>
DatasetViewImpl<Ts...>::DatasetViewImpl(
    MaybeConstDataset<Ts...> &dataset,
    const std::set<Dimension> &fixedDimensions)
    : m_units{UnitHelper<Ts>::get(dataset)...},
      m_variables(makeVariables(dataset, fixedDimensions)) {}

template <class... Ts>
DatasetViewImpl<Ts...>::DatasetViewImpl(
    const DatasetViewImpl &other, const std::tuple<ref_type_t<Ts>...> &data)
    : m_units(other.m_units),
      m_variables(std::get<0>(other.m_variables),
                  std::get<1>(other.m_variables), data) {}

template <class... Ts>
using makeVariableReturnType = std::tuple<const gsl::index, const MultiIndex,
                                          const std::tuple<ref_type_t<Ts>...>>;

template <class... Ts>
makeVariableReturnType<Ts...> DatasetViewImpl<Ts...>::makeVariables(
    MaybeConstDataset<Ts...> &dataset, const std::string &name,
    const std::set<Dimension> &fixedDimensions) const {
  boost::container::small_vector<Dimensions, 4> subdimensions{
      DimensionHelper<Ts>::get(dataset, name, fixedDimensions)...};
  Dimensions iterationDimensions(
      relevantDimensions(dataset, subdimensions, fixedDimensions));
  return std::tuple<const gsl::index, const MultiIndex,
                    const std::tuple<ref_type_t<Ts>...>>{
      iterationDimensions.volume(),
      MultiIndex(iterationDimensions, subdimensions),
      std::tuple<ref_type_t<Ts>...>{
          DataHelper<Ts>::get(dataset, iterationDimensions, name)...}};
}
template <class... Ts>
makeVariableReturnType<Ts...> DatasetViewImpl<Ts...>::makeVariables(
    MaybeConstDataset<Ts...> &dataset,
    const std::set<Dimension> &fixedDimensions) const {
  boost::container::small_vector<Dimensions, 4> subdimensions{
      DimensionHelper<Ts>::get(dataset, fixedDimensions)...};
  Dimensions iterationDimensions(
      relevantDimensions(dataset, subdimensions, fixedDimensions));
  return std::tuple<const gsl::index, const MultiIndex,
                    const std::tuple<ref_type_t<Ts>...>>{
      iterationDimensions.volume(),
      MultiIndex(iterationDimensions, subdimensions),
      std::tuple<ref_type_t<Ts>...>{
          DataHelper<Ts>::get(dataset, iterationDimensions)...}};
}

#define INSTANTIATE(...) template class DatasetViewImpl<__VA_ARGS__>;

// For very special cases we should probably provide a header that includes the
// definition, so we do not need to instantiate everything directly, but just
// the most common cases.
INSTANTIATE(Bin<Coord::Tof>)
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
