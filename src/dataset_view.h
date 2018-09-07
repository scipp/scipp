/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DATASET_VIEW_H
#define DATASET_VIEW_H

#include <algorithm>
#include <set>
#include <tuple>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/sort.hpp>
#include <boost/mpl/vector_c.hpp>

#include "dataset.h"
#include "multi_index.h"

namespace detail {
template <class T> struct value_type { using type = T; };
template <class T> struct value_type<Bin<T>> { using type = const T; };
template <class T> using value_type_t = typename value_type<T>::type;

template <class T> struct is_bins : public std::false_type {};
template <class T> struct is_bins<Bin<T>> : public std::true_type {};
template <class T> using is_bins_t = typename is_bins<T>::type;

template <class Tag> struct unit { using type = Unit; };
template <class... Tags> struct unit<DatasetViewImpl<Tags...>> {
  using type = std::tuple<typename unit<Tags>::type...>;
};
template <class... Tags> using unit_t = typename unit<Tags...>::type;
}

template <class Base, class T> struct GetterMixin {};

#define GETTER_MIXIN(Tag, name)                                                \
  template <class Base> struct GetterMixin<Base, Tag> {                        \
    const element_return_type_t<Tag> name() const {                            \
      return static_cast<const Base *>(this)->template get<Tag>();             \
    }                                                                          \
  };                                                                           \
  template <class Base> struct GetterMixin<Base, const Tag> {                  \
    const element_return_type_t<const Tag> name() const {                      \
      return static_cast<const Base *>(this)->template get<Tag>();             \
    }                                                                          \
  };

GETTER_MIXIN(Coord::Tof, tof)
GETTER_MIXIN(Data::Tof, tof)
GETTER_MIXIN(Data::Value, value)
GETTER_MIXIN(Data::Variance, variance)

template <class Base, class T> struct GetterMixin<Base, Bin<T>> {
  // Lift the getters of Bin into the iterator.
  double left() const {
    return static_cast<const Base *>(this)->template get<Bin<T>>().left();
  }
  double right() const {
    return static_cast<const Base *>(this)->template get<Bin<T>>().right();
  }
};

template <class Tag> struct ref_type {
  using type = gsl::span<std::conditional_t<
      std::is_const<Tag>::value, const typename detail::value_type_t<Tag>::type,
      typename detail::value_type_t<Tag>::type>>;
};
template <class Tag> struct ref_type<Bin<Tag>> {
  // First is the offset to the next edge.
  using type =
      std::pair<gsl::index,
                gsl::span<const typename detail::value_type_t<Bin<Tag>>::type>>;
};
template <> struct ref_type<Coord::SpectrumPosition> {
  using type =
      std::pair<gsl::span<const typename Coord::DetectorPosition::type>,
                gsl::span<const typename Coord::DetectorGrouping::type>>;
};
template <> struct ref_type<Data::StdDev> {
  using type = typename ref_type<const Data::Variance>::type;
};
template <class... Tags> struct ref_type<DatasetViewImpl<Tags...>> {
  using type = std::tuple<const MultiIndex, const DatasetViewImpl<Tags...>,
                          std::tuple<typename ref_type<Tags>::type...>>;
};
template <class T> using ref_type_t = typename ref_type<T>::type;

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
};

template <> struct UnitHelper<Coord::SpectrumPosition> {
  static Unit get(const Dataset &dataset) {
    return dataset.unit<Coord::DetectorPosition>();
  }
};

template <> struct UnitHelper<Data::StdDev> {
  static Unit get(const Dataset &dataset) {
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
};

template <> struct DimensionHelper<Coord::SpectrumPosition> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions);
};

template <> struct DimensionHelper<Data::StdDev> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions);
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

namespace detail {
template <class... Conds> struct and_ : std::true_type {};
template <class Cond, class... Conds>
struct and_<Cond, Conds...>
    : std::conditional<Cond::value, and_<Conds...>, std::false_type>::type {};

template <class T> struct is_const : std::false_type {};
template <class T> struct is_const<const T> : std::true_type {};
template <class... Ts>
struct is_const<DatasetViewImpl<Ts...>> : and_<is_const<Ts>...> {};
}

template <class... Tags>
using MaybeConstDataset =
    std::conditional_t<detail::and_<detail::is_const<Tags>...>::value,
                       const Dataset, Dataset>;

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
};

template <> struct DataHelper<Coord::SpectrumPosition> {
  static auto get(const Dataset &dataset,
                  const Dimensions &iterationDimensions) {
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

template <class Tag> struct SubdataHelper {
  static auto get(const ref_type_t<Tag> &data, const gsl::index offset) {
    return data.subspan(offset);
  }
};

template <class Tag> struct SubdataHelper<Bin<Tag>> {
  static auto get(const ref_type_t<Bin<Tag>> &data, const gsl::index offset) {
    return ref_type_t<Bin<Tag>>{data.first, data.second.subspan(offset)};
  }
};

/// Class with overloads used to handle "virtual" variables such as
/// Coord::SpectrumPosition.
template <class Tag> struct ItemHelper {
  static element_return_type_t<Tag> get(const ref_type_t<Tag> &data,
                                        gsl::index index) {
    return data[index];
  }
};

template <> struct ItemHelper<Coord::SpectrumPosition> {
  static element_return_type_t<Coord::SpectrumPosition>
  get(const ref_type_t<Coord::SpectrumPosition> &data, gsl::index index) {
    if (data.second[index].empty())
      throw std::runtime_error(
          "Spectrum has no detectors, cannot get position.");
    double position = 0.0;
    for (const auto det : data.second[index])
      position += data.first[det];
    return position /= data.second[index].size();
  }
};

template <> struct ItemHelper<Data::StdDev> {
  static element_return_type_t<Data::StdDev>
  get(const ref_type_t<Data::StdDev> &data, gsl::index index) {
    return sqrt(ItemHelper<const Data::Variance>::get(data, index));
  }
};

template <class Tag> struct ItemHelper<Bin<Tag>> {
  static element_return_type_t<Bin<Tag>> get(const ref_type_t<Bin<Tag>> &data,
                                             gsl::index index) {
    auto offset = data.first;
    return DataBin(data.second[index], data.second[index + offset]);
  }
};

template <class... Tags> struct ItemHelper<DatasetViewImpl<Tags...>> {
  template <class Tag>
  static constexpr auto subindex =
      detail::index<Tag, std::tuple<Tags...>>::value;

  static element_return_type_t<DatasetViewImpl<Tags...>>
  get(const ref_type_t<DatasetViewImpl<Tags...>> &data, gsl::index index) {
    // Add offset to each span passed to the nested DatasetView.
    MultiIndex nestedIndex = std::get<0>(data);
    nestedIndex.setIndex(index);
    auto subdata = std::make_tuple(
        SubdataHelper<Tags>::get(std::get<subindex<Tags>>(std::get<2>(data)),
                                 nestedIndex.get<subindex<Tags>>())...);
    return DatasetViewImpl<Tags...>(std::get<1>(data), subdata);
  }
};

template <class... Ts> class DatasetViewImpl {
  static_assert(sizeof...(Ts),
                "DatasetView requires at least one variable for iteration");

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
      const std::set<Dimension> &fixedDimensions) const;

public:
  class iterator;
  class Item : public GetterMixin<Item, Ts>... {
  public:
    Item(const gsl::index index, const MultiIndex &multiIndex,
         const std::tuple<ref_type_t<Ts>...> &variables)
        : m_index(multiIndex), m_variables(variables) {
      setIndex(index);
    }

    template <class Tag> element_return_type_t<maybe_const<Tag>> get() const {
      // Should we allow passing const?
      static_assert(!std::is_const<Tag>::value, "Do not use `const` qualifier "
                                                "for tags when accessing "
                                                "DatasetView::iterator.");
      constexpr auto variableIndex = tag_index<Tag>;
      return ItemHelper<maybe_const<Tag>>::get(
          std::get<variableIndex>(m_variables), m_index.get<variableIndex>());
    }

  private:
    friend class iterator;
    // Private such that iterator can be copied but clients cannot extract Item
    // (access only by reference).
    Item(const Item &other) = default;
    void setIndex(const gsl::index index) { m_index.setIndex(index); }

    bool operator==(const Item &other) const {
      return m_index == other.m_index;
    }

    MultiIndex m_index;
    const std::tuple<ref_type_t<Ts>...> &m_variables;
  };

  class iterator
      : public boost::iterator_facade<iterator, const Item,
                                      boost::random_access_traversal_tag> {
  public:
    iterator(const gsl::index index, const MultiIndex &multiIndex,
             const std::tuple<ref_type_t<Ts>...> &variables)
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

  DatasetViewImpl(MaybeConstDataset<Ts...> &dataset, const std::string &name,
                  const std::set<Dimension> &fixedDimensions = {})
      : m_units{UnitHelper<Ts>::get(dataset, name)...},
        m_variables(makeVariables(dataset, name, fixedDimensions)) {}
  DatasetViewImpl(MaybeConstDataset<Ts...> &dataset,
                  const std::set<Dimension> &fixedDimensions = {})
      : m_units{UnitHelper<Ts>::get(dataset)...},
        m_variables(makeVariables(dataset, fixedDimensions)) {}

  DatasetViewImpl(const DatasetViewImpl &other,
                  const std::tuple<ref_type_t<Ts>...> &data)
      : m_units(other.m_units),
        m_variables(std::get<0>(other.m_variables),
                    std::get<1>(other.m_variables), data) {}

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
             const std::tuple<ref_type_t<Ts>...>>
  makeVariables(MaybeConstDataset<Ts...> &dataset, const std::string &name,
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
  std::tuple<const gsl::index, const MultiIndex,
             const std::tuple<ref_type_t<Ts>...>>
  makeVariables(MaybeConstDataset<Ts...> &dataset,
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

  const std::tuple<detail::unit_t<Ts>...> m_units;
  const std::tuple<const gsl::index, const MultiIndex,
                   const std::tuple<ref_type_t<Ts>...>> m_variables;
};

namespace detail {
// Helpers used for making DatasetView independent of the order used when
// specifying tags.
template <class T> struct type_to_id {
  static constexpr int32_t value =
      std::is_const<T>::value ? 4 * tag_id<T> + 1 : 4 * tag_id<T> + 3;
};
template <class T> struct type_to_id<Bin<T>> {
  static constexpr int32_t value =
      std::is_const<T>::value ? 4 * tag_id<T> + 2 : 4 * tag_id<T> + 4;
};
// Nested DatasetView gets an ID based on the IDs of all child tags.
template <class... Ts> struct type_to_id<DatasetViewImpl<Ts...>> {
  static constexpr std::array<int32_t, sizeof...(Ts)> ids{
      type_to_id<Ts>::value...};
  static constexpr int32_t value =
      200 * ((sizeof...(Ts) == 1) ? ids[0] : (sizeof...(Ts) == 2)
                                                 ? 200 * ids[1] + ids[0]
                                                 : 200 * 200 * ids[2] +
                                                       200 * ids[1] + ids[0]);
};

template <int32_t N>
using get_type = std::conditional_t<
    N % 2 == 0,
    std::conditional_t<N % 4 == 0, Bin<std::tuple_element_t<(N - 1) / 4, Tags>>,
                       const Bin<std::tuple_element_t<(N - 1) / 4, Tags>>>,
    std::conditional_t<N % 4 == 3, std::tuple_element_t<(N - 1) / 4, Tags>,
                       const std::tuple_element_t<(N - 1) / 4, Tags>>>;

template <int32_t N> struct id_to_type {
  using type = std::conditional_t<
      (N < 200), get_type<N % 200>,
      std::conditional_t<
          (N < 200 * 200), std::tuple<get_type<(N / 200) % 200>>,
          std::conditional_t<
              (N < 200 * 200 * 200),
              std::tuple<get_type<(N / 200) % 200>,
                         get_type<(N / (200 * 200)) % 200>>,
              std::tuple<get_type<(N / 200) % 200>,
                         get_type<(N / (200 * 200)) % 200>,
                         get_type<(N / (200 * 200 * 200)) % 200>>>>>;
};
template <int32_t N> using id_to_type_t = typename id_to_type<N>::type;

template <class Sorted, size_t... Is>
auto sort_types_impl(std::index_sequence<Is...>) {
  return std::tuple<
      id_to_type_t<boost::mpl::at_c<Sorted, Is>::type::value>...>();
}

template <class... Ts> auto sort_types() {
  using Unsorted = boost::mpl::vector_c<int, type_to_id<Ts>::value...>;
  return sort_types_impl<typename boost::mpl::sort<Unsorted>::type>(
      std::make_index_sequence<sizeof...(Ts)>{});
}

// Helper to return either Tag or translate tuple of tags into DatasetView.
template <class Tag> struct tag { using type = Tag; };
template <class... Tags> struct tag<std::tuple<Tags...>> {
  using type = DatasetViewImpl<Tags...>;
};

// Helper to translate (potentially nested) tuple of Tags into DatasetView.
template <class T> struct dataset_view;
template <class... Ts> struct dataset_view<std::tuple<Ts...>> {
  using type = DatasetViewImpl<typename tag<Ts>::type...>;
};
}

template <class... Ts>
using DatasetView =
    typename detail::dataset_view<decltype(detail::sort_types<Ts...>())>::type;

#endif // DATASET_VIEW_H
