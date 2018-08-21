#ifndef DATASET_VIEW_H
#define DATASET_VIEW_H

#include <algorithm>
#include <set>
#include <tuple>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>

#include "dataset.h"
#include "histogram.h"
#include "multi_index.h"

template <class T> struct Slab { using value_type = T; };

namespace detail {
template <class T> struct value_type { using type = T; };
template <class T> struct value_type<Bin<T>> { using type = T; };
template <class T> struct value_type<Slab<T>> {
  using type = typename Slab<T>::value_type;
};
template <class T> struct value_type<const Slab<T>> {
  using type = const typename Slab<T>::value_type;
};
template <class T> using value_type_t = typename value_type<T>::type;

template <class T> struct is_slab : public std::false_type {};
template <class T> struct is_slab<Slab<T>> : public std::true_type {};
template <class T> using is_slab_t = typename is_slab<T>::type;

template <class T> struct is_bins : public std::false_type {};
template <class T> struct is_bins<Bin<T>> : public std::true_type {};
template <class T> using is_bins_t = typename is_bins<T>::type;

template <class Tag> struct unit { using type = Unit; };
template <class... Tags> struct unit<DatasetView<Tags...>> {
  using type = std::tuple<typename unit<Tags>::type...>;
};
template <class... Tags> using unit_t = typename unit<Tags...>::type;
}

template <class Base, class T> struct GetterMixin {};

template <class Base> struct GetterMixin<Base, Data::Tof> {
  const element_return_type_t<Data::Tof> tof() const {
    return static_cast<const Base *>(this)->template get<Data::Tof>();
  }
};

template <class Base> struct GetterMixin<Base, Data::Value> {
  const element_return_type_t<Data::Value> value() const {
    return static_cast<const Base *>(this)->template get<Data::Value>();
  }
};

template <class Base> struct GetterMixin<Base, Data::Histogram> {
  const element_return_type_t<Data::Histogram> histogram() const {
    return static_cast<const Base *>(this)->template get<Data::Histogram>();
  }
};

template <class Tag> struct ref_type {
  using type = gsl::span<std::conditional_t<
      std::is_const<Tag>::value, const typename detail::value_type_t<Tag>::type,
      typename detail::value_type_t<Tag>::type>>;
};
template <> struct ref_type<Coord::SpectrumPosition> {
  using type = std::pair<gsl::span<typename Coord::DetectorPosition::type>,
                         gsl::span<typename Coord::DetectorGrouping::type>>;
};
template <class... Tags> struct ref_type<DatasetView<Tags...>> {
  using type = std::pair<DatasetView<Tags...>,
                         std::tuple<typename ref_type<Tags>::type...>>;
};
template <class T> using ref_type_t = typename ref_type<T>::type;

template <class Tag> struct UnitHelper {
  static Unit get(const Dataset &dataset) { return dataset.unit<Tag>(); }

  static Unit get(const Dataset &dataset, const std::string &name) {
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

template <class... Tags> struct UnitHelper<DatasetView<Tags...>> {
  static detail::unit_t<DatasetView<Tags...>> get(const Dataset &dataset) {
    return std::make_tuple(dataset.unit<Tags>()...);
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
    // TODO Use name only for non-coord variables.
    // TODO Do we need to check here if fixedDimensions are contained?
    return dataset.dimensions<Tag>(name);
  }
};

template <class Tag> struct DimensionHelper<Slab<Tag>> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    auto dims = dataset.dimensions<Tag>();
    for (const auto dim : fixedDimensions)
      dims.erase(dim);
    return dims;
  }
};

template <class Tag> struct DimensionHelper<Bin<Tag>> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    auto dims = dataset.dimensions<Tag>();
    // TODO make this work for multiple dimensions and ragged dimensions.
    dims.resize(dims.label(0), dims.size(0) - 1);
    return dims;
  }
};

template <> struct DimensionHelper<Data::Histogram> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    auto dims = dataset.dimensions<Data::Value>();
    if (fixedDimensions.size() != 1)
      throw std::runtime_error(
          "Bad number of fixed dimensions. Only 1D histograms are supported.");
    dims.erase(*fixedDimensions.begin());
    return dims;
  }
};

template <> struct DimensionHelper<Coord::SpectrumPosition> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    return dataset.dimensions<Coord::DetectorGrouping>();
  }
};

template <class Tag, class... Tags>
struct DimensionHelper<DatasetView<Tag, Tags...>> {
  static Dimensions get(const Dataset &dataset,
                        const std::set<Dimension> &fixedDimensions) {
    // TODO Support only 1D nested view with all dimensions identical? Assume
    // this is the case for:
    auto dims = dataset.dimensions<Tag>();
    // TODO I think this is wrong: We need the fixed dimensions when computing
    // offsets (but we do not want it when determining iteration?)
    // for (const auto dim : fixedDimensions)
    //  dims.erase(dim);
    return dims;
  }
};

template <class Tag> struct DataHelper {
  static auto get(Dataset &dataset, const Dimensions &iterationDimensions) {
    return dataset.get<detail::value_type_t<Tag>>();
  }
  static auto get(Dataset &dataset, const Dimensions &iterationDimensions,
                  const std::string &name) {
    return dataset.get<detail::value_type_t<Tag>>(name);
  }
};

template <> struct DataHelper<Coord::SpectrumPosition> {
  static auto get(Dataset &dataset, const Dimensions &iterationDimensions) {
    return ref_type_t<Coord::SpectrumPosition>(
        dataset.get<detail::value_type_t<Coord::DetectorPosition>>(),
        dataset.get<detail::value_type_t<Coord::DetectorGrouping>>());
  }
};

template <class... Tags> struct DataHelper<DatasetView<Tags...>> {
  static auto get(Dataset &dataset, const Dimensions &iterationDimensions) {
    std::set<Dimension> fixedDimensions;
    for (auto &item : iterationDimensions)
      fixedDimensions.insert(item.first);
    // For the nested case we create a DatasetView with the correct dimensions
    // and store it. It is later copied and initialized with the correct offset
    // in iterator::get.
    return ref_type_t<DatasetView<Tags...>>{
        DatasetView<Tags...>(dataset, fixedDimensions),
        std::make_tuple(dataset.get<detail::value_type_t<Tags>>()...)};
  }
};

/// Class with overloads used to handle "virtual" variables such as
/// Coord::SpectrumPosition.
template <class Tag> struct ItemHelper {
  static element_return_type_t<Tag> get(ref_type_t<Tag> &data,
                                        gsl::index index) {
    return data[index];
  }
};

template <> struct ItemHelper<Coord::SpectrumPosition> {
  static element_return_type_t<Coord::SpectrumPosition>
  get(ref_type_t<Coord::SpectrumPosition> &data, gsl::index index) {
    if (data.second[index].empty())
      throw std::runtime_error(
          "Spectrum has no detectors, cannot get position.");
    double position = 0.0;
    for (const auto det : data.second[index])
      position += data.first[det];
    return position /= data.second[index].size();
  }
};

template <class... Tags> struct ItemHelper<DatasetView<Tags...>> {
  static element_return_type_t<DatasetView<Tags...>>
  get(ref_type_t<DatasetView<Tags...>> &data, gsl::index index) {
    // Add offset to each span passed to the nested DatasetView.
    auto subdata = std::make_tuple(
        std::get<detail::index<Tags, std::tuple<Tags...>>::value>(data.second)
            .subspan(index)...);
    return DatasetView<Tags...>(data.first, subdata);
  }
};

// pass non-iterated dimensions in constructor?
// Dataset::begin(Dimension::Tof)??
// Dataset::begin(DoNotIterate::Tof)??
template <class... Ts>
class DatasetView : public GetterMixin<DatasetView<Ts...>, Ts>... {
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

  Dimensions
  relevantDimensions(const std::vector<Dimensions> &variableDimensions,
                     const std::set<Dimension> &fixedDimensions) {
    auto largest =
        *std::max_element(variableDimensions.begin(), variableDimensions.end(),
                          [](const Dimensions &a, const Dimensions &b) {
                            return a.count() < b.count();
                          });
    for (const auto dim : fixedDimensions)
      if (largest.contains(dim))
        largest.erase(dim);

    std::vector<bool> is_const{std::is_const<Ts>::value...};
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

public:
  class iterator;
  class Item : public GetterMixin<Item, Ts>... {
  public:
    Item(const gsl::index index, const Dimensions &dimensions,
         const std::vector<Dimensions> &subdimensions,
         std::tuple<ref_type_t<Ts>...> &variables)
        : m_index(dimensions, subdimensions), m_variables(variables) {
      setIndex(index);
    }

    template <class Tag>
    element_return_type_t<maybe_const<Tag>>
    get(std::enable_if_t<!detail::is_bins<Tag>::value> * = nullptr) const {
      // Should we allow passing const?
      static_assert(!std::is_const<Tag>::value, "Do not use `const` qualifier "
                                                "for tags when accessing "
                                                "DatasetView::iterator.");
      constexpr auto variableIndex = tag_index<Tag>;
      auto &col = std::get<variableIndex>(m_variables);

      // TODO Ensure that this is inlined and does not affect performance.
      return ItemHelper<std::tuple_element_t<
          variableIndex, std::tuple<Ts...>>>::get(col,
                                                  m_index.get<variableIndex>());
    }

    template <class Tag>
    auto get(std::enable_if_t<detail::is_bins<Tag>::value> * = nullptr) const {
      static_assert(!std::is_const<Tag>::value, "Do not use `const` qualifier "
                                                "for tags when accessing "
                                                "DatasetView::iterator.");
      constexpr auto variableIndex = tag_index<Tag>;
      auto &col = std::get<variableIndex>(m_variables);
      // TODO This is wrong if bins are not the innermost index. Ensure that
      // that is always the case at time of creation?
      return DataBin(col[m_index.get<variableIndex>()],
                     col[m_index.get<variableIndex>() + 1]);
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
    std::tuple<ref_type_t<Ts>...> &m_variables;
  };

  class iterator
      : public boost::iterator_facade<iterator, const Item,
                                      boost::random_access_traversal_tag> {
  public:
    iterator(const gsl::index index, const Dimensions &dimensions,
             const std::vector<Dimensions> &subdimensions,
             std::tuple<ref_type_t<Ts>...> &variables)
        : m_item(index, dimensions, subdimensions, variables) {}

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

  DatasetView(Dataset &dataset, const std::string &name,
              const std::set<Dimension> &fixedDimensions = {})
      : m_fixedDimensions(fixedDimensions),
        m_units{UnitHelper<Ts>::get(dataset, name)...},
        m_subdimensions{
            DimensionHelper<Ts>::get(dataset, name, fixedDimensions)...},
        m_relevantDimensions(
            relevantDimensions(m_subdimensions, fixedDimensions)),
        m_columns(DataHelper<Ts>::get(dataset, m_relevantDimensions, name)...) {
  }
  DatasetView(Dataset &dataset, const std::set<Dimension> &fixedDimensions = {})
      : m_fixedDimensions(fixedDimensions),
        m_units{UnitHelper<Ts>::get(dataset)...},
        m_subdimensions{DimensionHelper<Ts>::get(dataset, fixedDimensions)...},
        m_relevantDimensions(
            relevantDimensions(m_subdimensions, fixedDimensions)),
        m_columns(DataHelper<Ts>::get(dataset, m_relevantDimensions)...) {}

  DatasetView(const DatasetView &other,
              const std::tuple<ref_type_t<Ts>...> &data)
      : m_fixedDimensions(other.m_fixedDimensions), m_units(other.m_units),
        m_subdimensions(other.m_subdimensions),
        m_relevantDimensions(other.m_relevantDimensions), m_columns(data) {}

  gsl::index size() const { return m_relevantDimensions.volume(); }

  iterator begin() {
    return {0, m_relevantDimensions, m_subdimensions, m_columns};
  }
  iterator end() {
    return {m_relevantDimensions.volume(), m_relevantDimensions,
            m_subdimensions, m_columns};
  }

private:
  const std::set<Dimension> m_fixedDimensions;
  const std::tuple<detail::unit_t<Ts>...> m_units;
  const std::vector<Dimensions> m_subdimensions;
  const Dimensions m_relevantDimensions;
  std::tuple<ref_type_t<Ts>...> m_columns;
};

#endif // DATASET_VIEW_H
