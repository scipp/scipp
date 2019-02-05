/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "md_zip_view.h"

using namespace detail;

template <class D, class Tag> struct UnitHelper {
  static Unit get(const Dataset &dataset,
                  const std::string &name = std::string{}) {
    if (is_coord<Tag>)
      return dataset(Tag{}).unit();
    else
      return dataset(Tag{}, name).unit();
  }
};

template <class D, class Tag> struct UnitHelper<D, Bin<Tag>> {
  static Unit get(const Dataset &dataset,
                  const std::string &name = std::string{}) {
    static_assert(is_coord<Tag>,
                  "Only coordinates can be defined at bin edges");
    static_cast<void>(name);
    return dataset(Tag{}).unit();
  }
};

template <class D> struct UnitHelper<D, const Coord::Position> {
  static Unit get(const Dataset &dataset,
                  const std::string &name = std::string{}) {
    static_cast<void>(name);
    if (dataset.contains(Coord::Position{}))
      return dataset(Coord::Position{}).unit();
    return dataset.get<const Coord::DetectorInfo>()[0](Coord::Position{})
        .unit();
  }
};

template <class D> struct UnitHelper<D, Data::StdDev> {
  static Unit get(const Dataset &dataset,
                  const std::string &name = std::string{}) {
    return dataset(Data::Variance{}, name).unit();
  }
};

template <class D, class... Tags>
struct UnitHelper<D, MDZipViewImpl<D, Tags...>> {
  static detail::unit_t<MDZipViewImpl<D, Tags...>>
  get(const Dataset &dataset, const std::string &name = std::string{}) {
    return std::make_tuple(UnitHelper<D, Tags>::get(dataset, name)...);
  }
};

template <class D, class Tag> struct DimensionHelper {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dim> &fixedDimensions,
                        const std::string &name = std::string{}) {
    static_cast<void>(fixedDimensions);
    // TODO Do we need to check here if fixedDimensions are contained?
    if (is_coord<Tag>)
      return dataset(Tag{}).dimensions();
    else
      return dataset(Tag{}, name).dimensions();
  }
};

template <class D, class Tag> struct DimensionHelper<D, Bin<Tag>> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dim> &fixedDimensions,
                        const std::string &name = std::string{}) {
    static_cast<void>(fixedDimensions);
    if (is_coord<Tag>)
      return dataset(Tag{}).dimensions();
    else
      return dataset(Tag{}, name).dimensions();
  }
};

template <class D> struct DimensionHelper<D, const Coord::Position> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dim> &fixedDimensions,
                        const std::string &name = std::string{}) {
    static_cast<void>(fixedDimensions);
    static_cast<void>(name);
    if (dataset.contains(Coord::Position{}))
      return dataset(Coord::Position{}).dimensions();
    // Note: We do *not* return the dimensions of the nested positions in
    // Coord::DetectorInfo since those are not dimensions of the dataset.
    return dataset(Coord::DetectorGrouping{}).dimensions();
  }
};

template <class D> struct DimensionHelper<D, Data::StdDev> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dim> &fixedDimensions,
                        const std::string &name = std::string{}) {
    static_cast<void>(fixedDimensions);
    return dataset(Data::Variance{}, name).dimensions();
  }
};

template <class D, class... Tags>
struct DimensionHelper<D, MDZipViewImpl<D, Tags...>> {
  static Dimensions getHelper(std::vector<Dimensions> variableDimensions,
                              const std::set<Dim> &fixedDimensions) {
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
    for (size_t i = 0; i < sizeof...(Tags); ++i) {
      auto dims = variableDimensions[i];
      if (!((largest == dims) || is_const[i]))
        throw std::runtime_error("Variables requested for iteration have "
                                 "different dimensions");
    }
    return largest;
  }

  static Dimensions get(const Dataset &dataset,
                        const std::set<Dim> &fixedDimensions,
                        const std::string &name = std::string{}) {
    return getHelper(
        {DimensionHelper<D, Tags>::get(dataset, fixedDimensions, name)...},
        fixedDimensions);
  }
};

template <class D, class Tag> struct DataHelper {
  static auto get(D &dataset, const Dimensions &,
                  const std::string &name = std::string{}) {
    if (is_coord<Tag>)
      return dataset.template get<detail::value_type_t<Tag>>();
    else
      return dataset.template get<detail::value_type_t<Tag>>(name);
  }
};

template <class D, class Tag> struct DataHelper<D, Bin<Tag>> {
  static auto get(const Dataset &dataset, const Dimensions &iterationDimensions,
                  const std::string &name = std::string{}) {
    static_cast<void>(iterationDimensions);
    static_cast<void>(name);
    // Compute offset to next edge.
    gsl::index offset = 1;
    const auto &dims = dataset(Tag{}).dimensions();
    const auto &actual = dataset.dimensions();
    for (gsl::index i = dims.ndim() - 1; i >= 0; --i) {
      if (dims.size(i) != actual[dims.label(i)])
        break;
      offset *= dims.size(i);
    }

    return ref_type_t<D, Bin<Tag>>{
        offset, dataset.get<const detail::value_type_t<Tag>>()};
  }
};

template <class D> struct DataHelper<D, const Coord::Position> {
  static auto get(const Dataset &dataset, const Dimensions &,
                  const std::string &name = std::string{}) {
    static_cast<void>(name);
    // TODO Probably we should throw if there is Coord::Position as well as
    // Coord::DetectorGrouping/Coord::DetectorInfo. We should never have both, I
    // think.
    if (dataset.contains(Coord::Position{}))
      return ref_type_t<D, const Coord::Position>(
          dataset.get<detail::value_type_t<Coord::Position>>(),
          gsl::span<const typename Coord::DetectorGrouping::type>{});
    const auto &detInfo = dataset.get<Coord::DetectorInfo>()[0];
    return ref_type_t<D, const Coord::Position>(
        detInfo.get<detail::value_type_t<Coord::Position>>(),
        dataset.get<detail::value_type_t<Coord::DetectorGrouping>>());
  }
};

template <class D> struct DataHelper<D, Data::StdDev> {
  static auto get(D &dataset, const Dimensions &iterationDimensions,
                  const std::string &name = std::string{}) {
    return DataHelper<D, Data::Variance>::get(dataset, iterationDimensions,
                                              name);
  }
};

template <class D, class... Tags>
struct DataHelper<D, MDZipViewImpl<D, Tags...>> {
  static auto get(D &dataset, const Dimensions &iterationDimensions,
                  const std::string &name = std::string{}) {
    const auto labels = iterationDimensions.labels();
    std::set<Dim> fixedDimensions(labels.begin(), labels.end());
    // For the nested case we create a MDZipView with the correct dimensions
    // and store it. It is later copied and initialized with the correct offset
    // in iterator::get.
    return ref_type_t<D, MDZipViewImpl<D, Tags...>>{
        MultiIndex(iterationDimensions,
                   {DimensionHelper<D, Tags>::get(dataset, {}, name)...}),
        MDZipViewImpl<D, Tags...>(dataset, name, fixedDimensions),
        std::make_tuple(DataHelper<D, Tags>::get(dataset, {}, name)...)};
  }
};

template <class D, class... Ts>
Dimensions MDZipViewImpl<D, Ts...>::relevantDimensions(
    const Dataset &dataset,
    boost::container::small_vector<Dimensions, 4> variableDimensions,
    const std::set<Dim> &fixedDimensions) const {
  // The dimensions for the variables may be longer by one if the variable is
  // an edge variable. For iteration dimensions we require the dimensions
  // without the extended length. The original variableDimensions is kept
  // (note pass by value) since the extended length is required to compute the
  // correct offset into the variable.
  std::vector<bool> is_bins{detail::is_bins_t<Ts>::value...};
  for (size_t i = 0; i < sizeof...(Ts); ++i) {
    auto &dims = variableDimensions[i];
    if (is_bins[i]) {
      const auto &actual = dataset.dimensions();
      for (auto &dim : dims.labels())
        dims.resize(dim, actual[dim]);
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
  for (size_t i = 0; i < sizeof...(Ts); ++i) {
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

template <class D, class... Ts>
MDZipViewImpl<D, Ts...>::MDZipViewImpl(D &dataset, const std::string &name,
                                       const std::set<Dim> &fixedDimensions)
    : m_units{UnitHelper<D, Ts>::get(dataset, name)...},
      m_variables(makeVariables(dataset, fixedDimensions, name)) {}
template <class D, class... Ts>
MDZipViewImpl<D, Ts...>::MDZipViewImpl(D &dataset,
                                       const std::set<Dim> &fixedDimensions)
    : m_units{UnitHelper<D, Ts>::get(dataset)...},
      m_variables(makeVariables(dataset, fixedDimensions)) {}
template <class D, class... Ts>
MDZipViewImpl<D, Ts...>::MDZipViewImpl(
    D &dataset, const std::initializer_list<Dim> &fixedDimensions)
    : m_units{UnitHelper<D, Ts>::get(dataset)...},
      m_variables(makeVariables(dataset, fixedDimensions)) {}

template <class D, class... Ts>
MDZipViewImpl<D, Ts...>::MDZipViewImpl(
    const MDZipViewImpl &other, const std::tuple<ref_type_t<D, Ts>...> &data)
    : m_units(other.m_units),
      m_variables(std::get<0>(other.m_variables),
                  std::get<1>(other.m_variables), data) {}

template <class D, class... Ts>
using makeVariableReturnType =
    std::tuple<const gsl::index, const MultiIndex,
               const std::tuple<ref_type_t<D, Ts>...>>;

template <class D, class... Ts>
makeVariableReturnType<D, Ts...>
MDZipViewImpl<D, Ts...>::makeVariables(D &dataset,
                                       const std::set<Dim> &fixedDimensions,
                                       const std::string &name) const {
  boost::container::small_vector<Dimensions, 4> subdimensions{
      DimensionHelper<D, Ts>::get(dataset, fixedDimensions, name)...};
  Dimensions iterationDimensions(
      relevantDimensions(dataset, subdimensions, fixedDimensions));
  return std::tuple<const gsl::index, const MultiIndex,
                    const std::tuple<ref_type_t<D, Ts>...>>{
      iterationDimensions.volume(),
      MultiIndex(iterationDimensions, subdimensions),
      std::tuple<ref_type_t<D, Ts>...>{
          DataHelper<D, Ts>::get(dataset, iterationDimensions, name)...}};
}

#define INSTANTIATE(...)                                                       \
  template class MDZipViewImpl<Dataset, __VA_ARGS__>;                          \
  template class MDZipViewImpl<const Dataset, __VA_ARGS__>;
#define INSTANTIATE_MUTABLE(...)                                               \
  template class MDZipViewImpl<Dataset, __VA_ARGS__>;
#define INSTANTIATE_CONST(...)                                                 \
  template class MDZipViewImpl<const Dataset, __VA_ARGS__>;

// For very special cases we should probably provide a header that includes the
// definition, so we do not need to instantiate everything directly, but just
// the most common cases.
INSTANTIATE(Bin<Coord::Tof>)
INSTANTIATE(Bin<Coord::Tof>, Data::Int)
INSTANTIATE(Bin<Coord::Tof>, Data::Value, Data::Variance)
INSTANTIATE(Bin<Coord::X>)
INSTANTIATE(Bin<Coord::Y>)
INSTANTIATE_MUTABLE(Coord::Mask, MDZipViewImpl<Dataset, Data::Value>)
INSTANTIATE_MUTABLE(Coord::Mask const, MDZipViewImpl<Dataset, Data::Value>)
INSTANTIATE(Coord::Mask, Data::Value, Data::Variance)
INSTANTIATE(Coord::Mask const, Data::Value, Data::Variance)
INSTANTIATE_MUTABLE(Coord::SpectrumNumber const,
                    MDZipViewImpl<Dataset, Coord::Temperature const,
                                  Data::Value const, Data::Variance const>)
INSTANTIATE_MUTABLE(
    Coord::SpectrumNumber,
    MDZipViewImpl<Dataset, Bin<Coord::Tof>, Data::Value, Data::Variance>)
INSTANTIATE_MUTABLE(
    Coord::SpectrumNumber const,
    MDZipViewImpl<Dataset, Bin<Coord::Tof>, Data::Value, Data::Variance>)
INSTANTIATE(Coord::Position)
INSTANTIATE(Coord::Position const)
INSTANTIATE(Coord::Temperature, Data::Value, Data::Variance)
INSTANTIATE(Coord::Temperature const, Data::Value, Data::Variance)
INSTANTIATE(Coord::Temperature const, Data::Value const, Data::Variance const)
INSTANTIATE(Coord::Tof)
INSTANTIATE(Coord::Tof, Data::Int)
INSTANTIATE(Coord::X)
INSTANTIATE(Coord::X, Coord::Y)
INSTANTIATE(Coord::X const, Coord::Y)
INSTANTIATE(Coord::X, Data::Value)
INSTANTIATE(Coord::X, Data::Value const)
INSTANTIATE(Coord::X const, Data::Value const)
INSTANTIATE_MUTABLE(Coord::X, MDZipViewImpl<Dataset, Coord::Y>)
INSTANTIATE_MUTABLE(Coord::X, MDZipViewImpl<Dataset, Coord::Y, Coord::Z>)
INSTANTIATE(Coord::Y)
INSTANTIATE(Coord::Y, Coord::Z)
INSTANTIATE_MUTABLE(
    Data::Events const,
    MDZipViewImpl<Dataset, Bin<Coord::Tof>, Data::Value, Data::Variance>)
INSTANTIATE(Data::Int)
INSTANTIATE_MUTABLE(Data::Int, MDZipViewImpl<Dataset, Data::Value>)
INSTANTIATE_MUTABLE(Data::Int const, MDZipViewImpl<Dataset, Data::Value const>)
INSTANTIATE_CONST(Data::Int, MDZipViewImpl<const Dataset, Data::Value>)
INSTANTIATE_MUTABLE(Data::Int,
                    MDZipViewImpl<Dataset, Data::Value, Data::Variance>)
INSTANTIATE_MUTABLE(MDZipViewImpl<Dataset, Coord::X, Data::Value>)
INSTANTIATE_MUTABLE(MDZipViewImpl<Dataset, Coord::X, Data::Value const>)
INSTANTIATE_MUTABLE(MDZipViewImpl<Dataset, Coord::X const, Data::Value const>)
INSTANTIATE_MUTABLE(MDZipViewImpl<Dataset, Data::Value>)
INSTANTIATE_MUTABLE(MDZipViewImpl<Dataset, Data::Value const>)
INSTANTIATE_CONST(MDZipViewImpl<const Dataset, Data::Value>)
INSTANTIATE(Data::StdDev)
INSTANTIATE(Data::Value)
INSTANTIATE(Data::Value const)
INSTANTIATE(Data::Value, Data::String)
INSTANTIATE(Data::Value const, Data::String)
INSTANTIATE(Data::Value, Data::Int)
INSTANTIATE(Data::Value, Data::Int const)
INSTANTIATE(Data::Value, Data::Variance)
INSTANTIATE(Data::Value, Data::Variance const)
INSTANTIATE(Data::Variance, Data::Int)
INSTANTIATE(Coord::DetectorId const, Coord::Position)
INSTANTIATE(Coord::Ef, Coord::Position)
