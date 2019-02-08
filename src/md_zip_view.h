/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef MD_ZIP_VIEW_H
#define MD_ZIP_VIEW_H

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/sort.hpp>
#include <boost/mpl/vector_c.hpp>

#include "dataset.h"
#include "multi_index.h"
#include "traits.h"

namespace detail {
template <class T> struct value_type { using type = T; };
template <class T> struct value_type<Bin<T>> { using type = const T; };
template <class T> using value_type_t = typename value_type<T>::type;

template <class T> struct is_bins : public std::false_type {};
template <class T> struct is_bins<Bin<T>> : public std::true_type {};
template <class T> using is_bins_t = typename is_bins<T>::type;

template <class Tag> struct unit { using type = Unit; };
template <class D, class... Tags> struct unit<MDZipViewImpl<D, Tags...>> {
  using type = std::tuple<typename unit<Tags>::type...>;
};
template <class... Tags> using unit_t = typename unit<Tags...>::type;
} // namespace detail

template <class Base, class T> struct GetterMixin {};

// TODO Check const correctness here.
#define GETTER_MIXIN(Tag, name)                                                \
  template <class Base>                                                        \
  struct GetterMixin<Base, std::remove_cv_t<decltype(Tag)>> {                  \
    element_return_type_t<Dataset, std::remove_cv_t<decltype(Tag)>>            \
    name() const {                                                             \
      return static_cast<const Base *>(this)->template get(Tag);               \
    }                                                                          \
  };                                                                           \
  template <class Base>                                                        \
  struct GetterMixin<Base, const std::remove_cv_t<decltype(Tag)>> {            \
    element_return_type_t<const Dataset, std::remove_cv_t<decltype(Tag)>>      \
    name() const {                                                             \
      return static_cast<const Base *>(this)->template get(Tag);               \
    }                                                                          \
  };

GETTER_MIXIN(Coord::Tof, tof)
GETTER_MIXIN(Data::Tof, tof)
GETTER_MIXIN(Data::Value, value)
GETTER_MIXIN(Data::Variance, variance)

template <class Base, class T> struct GetterMixin<Base, Bin<T>> {
  // Lift the getters of Bin into the iterator.
  double left() const {
    return static_cast<const Base *>(this)->get(Bin<T>{}).left();
  }
  double right() const {
    return static_cast<const Base *>(this)->get(Bin<T>{}).right();
  }
};

template <class D, class Tag> struct ref_type {
  using type = gsl::span<std::conditional_t<
      std::is_const<D>::value, const typename detail::value_type_t<Tag>::type,
      typename detail::value_type_t<Tag>::type>>;
};
template <class D, class Tag> struct ref_type<D, Bin<Tag>> {
  // First is the offset to the next edge.
  using type =
      std::pair<gsl::index,
                gsl::span<const typename detail::value_type_t<Bin<Tag>>::type>>;
};

// TODO The need for the cumbersome std::remove_cv_t<decltype(...)> is legacy,
// we can probably refactor a lot of the helpers to be constexpr based on the
// value, rather then using detail::value_type_t, etc.
template <class D>
struct ref_type<D, const Coord::Position_t> {
  using type =
      std::pair<gsl::span<const typename Coord::Position_t::type>,
                gsl::span<const typename Coord::DetectorGrouping_t::type>>;
};
template <class D>
struct ref_type<D, Data::Events_t> {
  // Supporting either events stored as nested Dataset (Data::Events), or a
  // separate variables for tof and pulse-time (Data::EventTofs and
  // Data::EventPulseTimes).
  using type = std::tuple<gsl::span<typename Data::Events_t::type>,
                          gsl::span<typename Data::EventTofs_t::type>,
                          gsl::span<typename Data::EventPulseTimes_t::type>>;
};
template <class D> struct ref_type<D, Data::StdDev_t> {
  using type = typename ref_type<D, Data::Variance_t>::type;
};
template <class D, class... Tags>
struct ref_type<D, MDZipViewImpl<D, Tags...>> {
  using type = std::tuple<const MultiIndex, const MDZipViewImpl<D, Tags...>,
                          std::tuple<typename ref_type<D, Tags>::type...>>;
};
template <class D, class T> using ref_type_t = typename ref_type<D, T>::type;

template <class D, class Tag> struct SubdataHelper {
  static auto get(const ref_type_t<D, Tag> &data, const gsl::index offset) {
    return data.subspan(offset);
  }
};

template <class D, class Tag> struct SubdataHelper<D, Bin<Tag>> {
  static auto get(const ref_type_t<D, Bin<Tag>> &data,
                  const gsl::index offset) {
    return ref_type_t<D, Bin<Tag>>{data.first, data.second.subspan(offset)};
  }
};

/// Class with overloads used to handle "virtual" variables such as
/// Coord::Position.
template <class D, class Tag> struct ItemHelper {
  static element_return_type_t<D, Tag> get(const ref_type_t<D, Tag> &data,
                                           gsl::index index) {
    return data[index];
  }
};

// Note: Special case! Coord::Position can be either derived based on detectors,
// or stored directly.
template <class D> struct ItemHelper<D, const Coord::Position_t> {
  static element_return_type_t<D, const Coord::Position_t>
  get(const ref_type_t<D, const Coord::Position_t> &data, gsl::index index) {
    if (data.second.empty())
      return data.first[index];
    if (data.second[index].empty())
      throw std::runtime_error(
          "Spectrum has no detectors, cannot get position.");
    Eigen::Vector3d position{0.0, 0.0, 0.0};
    for (const auto det : data.second[index])
      position += data.first[det];
    return position /= static_cast<double>(data.second[index].size());
  }
};

template <class D> struct ItemHelper<D, Data::Events_t> {
  static element_return_type_t<D, Data::Events_t>
  get(const ref_type_t<D, Data::Events_t> &data, gsl::index index) {
    if (!std::get<1>(data).empty())
      return {std::get<1>(data)[index], std::get<2>(data)[index]};
    return {std::get<0>(data)[index]};
  }
};

template <class D>
struct ItemHelper<D, std::remove_cv_t<decltype(Data::StdDev)>> {
  static element_return_type_t<D, std::remove_cv_t<decltype(Data::StdDev)>>
  get(const ref_type_t<D, std::remove_cv_t<decltype(Data::StdDev)>> &data,
      gsl::index index) {
    return std::sqrt(
        ItemHelper<D, std::remove_cv_t<decltype(Data::Variance)>>::get(data,
                                                                       index));
  }
};

template <class D, class Tag> struct ItemHelper<D, Bin<Tag>> {
  static element_return_type_t<D, Bin<Tag>>
  get(const ref_type_t<D, Bin<Tag>> &data, gsl::index index) {
    auto offset = data.first;
    return DataBin(data.second[index], data.second[index + offset]);
  }
};

template <class D, class... Tags>
struct ItemHelper<D, MDZipViewImpl<D, Tags...>> {
  template <class Tag>
  static constexpr auto subindex =
      detail::index<Tag, std::tuple<Tags...>>::value;

  static element_return_type_t<D, MDZipViewImpl<D, Tags...>>
  get(const ref_type_t<D, MDZipViewImpl<D, Tags...>> &data, gsl::index index) {
    // Add offset to each span passed to the nested MDZipView.
    MultiIndex nestedIndex = std::get<0>(data);
    nestedIndex.setIndex(index);
    auto subdata = std::make_tuple(
        SubdataHelper<D, Tags>::get(std::get<subindex<Tags>>(std::get<2>(data)),
                                    nestedIndex.get<subindex<Tags>>())...);
    return MDZipViewImpl<D, Tags...>(std::get<1>(data), subdata);
  }
};

// MDLabelImpl is a helper for constructing a MDZipView.
template <class T, class TagT> struct MDLabelImpl {
  using type = T;
  using tag = TagT;
  const std::string name;
};

template <class TagT> auto MDRead(const TagT, const std::string &name = "") {
  if constexpr (detail::is_bins<TagT>::value)
    return MDLabelImpl<const typename TagT::type, TagT>{name};
  else if constexpr (std::is_same_v<std::remove_cv_t<decltype(Data::StdDev)>,
                                    TagT>)
    return MDLabelImpl<const typename TagT::type, TagT>{name};
  else
    return MDLabelImpl<const typename TagT::type, const TagT>{name};
}
template <class TagT> auto MDWrite(const TagT, const std::string &name = "") {
  return MDLabelImpl<typename TagT::type, TagT>{name};
}
template <class T, class TagT>
auto MDRead(const TagT, const std::string &name = "") {
  return MDLabelImpl<T,
                     std::conditional_t<std::is_const_v<T>, const TagT, TagT>>{
      name};
}

template <class D, class... Ts> class MDZipViewImpl {
  static_assert(sizeof...(Ts),
                "MDZipView requires at least one variable for iteration");

private:
  using tags = std::tuple<std::remove_const_t<Ts>...>;
  // Note: Not removing const from Tag, we want it to fail if const is passed.
  // TODO detail::index is from tags.h, put it somewhere else and rename.
  template <class Tag>
  static constexpr size_t tag_index = detail::index<Tag, tags>::value;
  template <class Tag>
  using maybe_const = std::tuple_element_t<tag_index<Tag>, std::tuple<Ts...>>;

  Dimensions relevantDimensions(
      const Dataset &dataset,
      boost::container::small_vector<Dimensions, 4> variableDimensions,
      const std::set<Dim> &fixedDimensions) const;

public:
  using type = MDZipViewImpl; // For nested MDLabelImpl.
  class iterator;
  class Item : public GetterMixin<Item, Ts>... {
  public:
    Item(const gsl::index index, const MultiIndex &multiIndex,
         const std::tuple<ref_type_t<D, Ts>...> &variables)
        : m_index(multiIndex), m_variables(&variables) {
      setIndex(index);
    }

    template <class TagT>
    element_return_type_t<D, maybe_const<TagT>> get(const TagT) const {
      // Should we allow passing const?
      static_assert(!std::is_const<TagT>::value, "Do not use `const` qualifier "
                                                "for tags when accessing "
                                                "MDZipView::iterator.");
      constexpr auto variableIndex = tag_index<TagT>;
      return ItemHelper<D, maybe_const<TagT>>::get(
          std::get<variableIndex>(*m_variables), m_index.get<variableIndex>());
    }

  private:
    friend class iterator;
    // Private such that iterator can be copied but clients cannot extract Item
    // (access only by reference).
    Item(const Item &) = default;
    Item &operator=(const Item &) = default;
    void setIndex(const gsl::index index) { m_index.setIndex(index); }

    bool operator==(const Item &other) const {
      return m_index == other.m_index;
    }

    MultiIndex m_index;
    const std::tuple<ref_type_t<D, Ts>...> *m_variables;
  };

  class iterator
      : public boost::iterator_facade<iterator, const Item,
                                      boost::random_access_traversal_tag> {
  public:
    iterator(const gsl::index index, const MultiIndex &multiIndex,
             const std::tuple<ref_type_t<D, Ts>...> &variables)
        : m_item(index, multiIndex, variables) {}

  private:
    friend class boost::iterator_core_access;

    bool equal(const iterator &other) const { return m_item == other.m_item; }
    void increment() { m_item.m_index.increment(); }
    const Item &dereference() const { return m_item; }
    void decrement() { m_item.setIndex(m_item.m_index.index() - 1); }

    void advance(int64_t delta) {
      if (delta == 1)
        increment();
      else
        m_item.setIndex(m_item.m_index.index() + delta);
    }

    int64_t distance_to(const iterator &other) const {
      return static_cast<int64_t>(other.m_item.m_index.index()) -
             static_cast<int64_t>(m_item.m_index.index());
    }

    Item m_item;
  };

  MDZipViewImpl(D &dataset, const std::string &name,
                const std::set<Dim> &fixedDimensions = {});
  MDZipViewImpl(D &dataset, const std::set<Dim> &fixedDimensions = {});
  MDZipViewImpl(D &dataset, const std::initializer_list<Dim> &fixedDimensions);

  MDZipViewImpl(const MDZipViewImpl &other,
                const std::tuple<ref_type_t<D, Ts>...> &data);

  gsl::index size() const { return std::get<0>(m_variables); }
  iterator begin() const {
    return {0, std::get<1>(m_variables), std::get<2>(m_variables)};
  }
  iterator end() const {
    return {std::get<0>(m_variables), std::get<1>(m_variables),
            std::get<2>(m_variables)};
  }

private:
  std::tuple<const gsl::index, const MultiIndex,
             const std::tuple<ref_type_t<D, Ts>...>>
  makeVariables(D &dataset, const std::set<Dim> &fixedDimensions,
                const std::string &name = std::string{}) const;

  const std::tuple<detail::unit_t<Ts>...> m_units;
  const std::tuple<const gsl::index, const MultiIndex,
                   const std::tuple<ref_type_t<D, Ts>...>>
      m_variables;
};

inline const std::string &commonName() {
  static std::string empty;
  return empty;
}
template <class T> const std::string &commonName(const T &label) {
  return label.name;
}

template <class T, class... Ts>
const std::string &commonName(const T &label, const Ts &... labels) {
  const auto &name = commonName(labels...);
  if (label.name.empty())
    return name;
  if (name.empty() || (name == label.name))
    return label.name;
  throw std::runtime_error(
      "MDZipView currently only supports a single variable name.");
}

template <class... Labels> auto zipMD(const Dataset &d, Labels... labels) {
  // TODO Currently this will only extract a single common name and the
  // consistency checking is not complete. Need to refactor MDZipView to support
  // multiple names.
  return MDZipViewImpl<const Dataset, typename Labels::tag...>(
      d, commonName(labels...));
}
template <class... Labels> auto zipMD(Dataset &d, Labels... labels) {
  return MDZipViewImpl<Dataset, typename Labels::tag...>(d,
                                                         commonName(labels...));
}

// TODO Can we put fixedDimensions into the label?
template <class... Labels>
auto zipMD(const Dataset &d, const std::initializer_list<Dim> &fixedDimensions,
           Labels... labels) {
  return MDZipViewImpl<const Dataset, typename Labels::tag...>(
      d, commonName(labels...), fixedDimensions);
}
template <class... Labels>
auto zipMD(Dataset &d, const std::initializer_list<Dim> &fixedDimensions,
           Labels... labels) {
  return MDZipViewImpl<Dataset, typename Labels::tag...>(
      d, commonName(labels...), fixedDimensions);
}

template <class... Labels> auto MDNested(Labels... labels) {
  Dataset d;
  using type = decltype(zipMD(d, labels...));
  return MDLabelImpl<type, type>{commonName(labels...)};
}

template <class... Labels> auto ConstMDNested(Labels... labels) {
  const Dataset d;
  using type = decltype(zipMD(d, labels...));
  return MDLabelImpl<type, type>{commonName(labels...)};
}

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

template <class D> struct UnitHelper<D, const Coord::Position_t> {
  static Unit get(const Dataset &dataset,
                  const std::string &name = std::string{}) {
    static_cast<void>(name);
    if (dataset.contains(Coord::Position))
      return dataset(Coord::Position).unit();
    return dataset.get(Coord::DetectorInfo)[0](Coord::Position).unit();
  }
};

template <class D> struct UnitHelper<D, Data::Events_t> {
  static Unit get(const Dataset &dataset,
                  const std::string &name = std::string{}) {
    static_cast<void>(name);
    if (dataset.contains(Data::Events))
      return dataset(Data::Events).unit();
    return Unit::Id::Dimensionless;
  }
};

template <class D> struct UnitHelper<D, std::remove_cv_t<decltype(Data::StdDev)>> {
  static Unit get(const Dataset &dataset,
                  const std::string &name = std::string{}) {
    return dataset(Data::Variance, name).unit();
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

template <class D> struct DimensionHelper<D, const Coord::Position_t> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dim> &fixedDimensions,
                        const std::string &name = std::string{}) {
    static_cast<void>(fixedDimensions);
    static_cast<void>(name);
    if (dataset.contains(Coord::Position))
      return dataset(Coord::Position).dimensions();
    // Note: We do *not* return the dimensions of the nested positions in
    // Coord::DetectorInfo since those are not dimensions of the dataset.
    return dataset(Coord::DetectorGrouping).dimensions();
  }
};

template <class D> struct DimensionHelper<D, Data::Events_t> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dim> &fixedDimensions,
                        const std::string &name = std::string{}) {
    static_cast<void>(fixedDimensions);
    static_cast<void>(name);
    if (dataset.contains(Data::Events))
      return dataset(Data::Events).dimensions();
    return dataset(Data::EventTofs).dimensions();
  }
};

template <class D>
struct DimensionHelper<D, std::remove_cv_t<decltype(Data::StdDev)>> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dim> &fixedDimensions,
                        const std::string &name = std::string{}) {
    static_cast<void>(fixedDimensions);
    return dataset(Data::Variance, name).dimensions();
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
      return dataset.get(detail::value_type_t<Tag>{});
    else
      return dataset.get(detail::value_type_t<Tag>{}, name);
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

    return ref_type_t<D, Bin<Tag>>{offset,
                                   dataset.get(detail::value_type_t<Tag>{})};
  }
};

template <class D> struct DataHelper<D, const Coord::Position_t> {
  static auto get(const Dataset &dataset, const Dimensions &,
                  const std::string &name = std::string{}) {
    static_cast<void>(name);
    // TODO Probably we should throw if there is Coord::Position as well as
    // Coord::DetectorGrouping/Coord::DetectorInfo. We should never have both, I
    // think.
    if (dataset.contains(Coord::Position))
      return ref_type_t<D, const Coord::Position_t>(
          dataset.get(detail::value_type_t<Coord::Position_t>{}),
          gsl::span<const typename Coord::DetectorGrouping_t::type>{});
    const auto &detInfo = dataset.get(Coord::DetectorInfo)[0];
    return ref_type_t<D, const Coord::Position_t>(
        detInfo.get(detail::value_type_t<Coord::Position_t>{}),
        dataset.get(detail::value_type_t<Coord::DetectorGrouping_t>{}));
  }
};

template <class D> struct DataHelper<D, Data::Events_t> {
  static auto get(Dataset &dataset, const Dimensions &,
                  const std::string &name = std::string{}) {
    static_cast<void>(name);
    if (dataset.contains(Data::Events)) {
      if (dataset.contains(Data::EventTofs))
        throw std::runtime_error("Cannot obtain events from dataset, contains "
                                 "conflicting information (Data::Events and "
                                 "Data::EventTofs).");
      return ref_type_t<D, Data::Events_t>(
          dataset.get(detail::value_type_t<Data::Events_t>{}),
          gsl::span<typename Data::EventTofs_t::type>{},
          gsl::span<typename Data::EventPulseTimes_t::type>{});
    }
    return ref_type_t<D, Data::Events_t>(
        gsl::span<typename Data::Events_t::type>{},
        dataset.get(detail::value_type_t<Data::EventTofs_t>{}),
        dataset.get(detail::value_type_t<Data::EventPulseTimes_t>{}));
  }
};

template <class D> struct DataHelper<D, std::remove_cv_t<decltype(Data::StdDev)>> {
  static auto get(D &dataset, const Dimensions &iterationDimensions,
                  const std::string &name = std::string{}) {
    return DataHelper<D, std::remove_cv_t<decltype(Data::Variance)>>::get(
        dataset, iterationDimensions, name);
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

#endif // MD_ZIP_VIEW_H
