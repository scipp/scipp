#ifndef DATASET_VIEW_H
#define DATASET_VIEW_H

#include <algorithm>
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
}

template <class Base, class T> struct GetterMixin {};

template <class Base> struct GetterMixin<Base, Data::Tof> {
  const element_reference_type_t<Data::Tof> tof() const {
    return static_cast<const Base *>(this)->template get<Data::Tof>();
  }
};

template <class Base> struct GetterMixin<Base, Data::Value> {
  const element_reference_type_t<Data::Value> value() const {
    return static_cast<const Base *>(this)->template get<Data::Value>();
  }
};

template <class Base> struct GetterMixin<Base, Data::Histogram> {
  const element_reference_type_t<Data::Histogram> histogram() const {
    return static_cast<const Base *>(this)->template get<Data::Histogram>();
  }
};

template <class Tag> struct ref_type {
  using type = gsl::span<std::conditional_t<
      std::is_const<Tag>::value, const typename detail::value_type_t<Tag>::type,
      typename detail::value_type_t<Tag>::type>>;
};
template <> struct ref_type<Coord::SpectrumPosition> {
  using type = std::pair<
      gsl::span<typename Coord::DetectorPosition::type>,
      gsl::span<typename Coord::DetectorGrouping::type>>;
};
template <class T> using ref_type_t = typename ref_type<T>::type;

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

template <class Tag>
std::unique_ptr<std::vector<Histogram>>
makeHistogramsIfRequired(Dataset &dataset) {
  return nullptr;
}

template <class Tag>
std::unique_ptr<std::vector<Histogram>>
makeHistogramsIfRequired(Dataset &dataset, const std::string &name) {
  return nullptr;
}

template <>
std::unique_ptr<std::vector<Histogram>>
makeHistogramsIfRequired<Data::Histogram>(Dataset &dataset) {
  auto histograms = std::make_unique<std::vector<Histogram>>(0);
  histograms->reserve(4);
  const auto edges = dataset.get<const Data::Tof>();
  auto values = dataset.get<Data::Value>();
  auto errors = dataset.get<Data::Error>();
  histograms->emplace_back(Unit::Id::Length, 2, 1, edges.data(), values.data(),
                           errors.data());
  histograms->emplace_back(Unit::Id::Length, 2, 1, edges.data() + 3,
                           values.data() + 2, errors.data() + 2);
  return histograms;
}

template <>
std::unique_ptr<std::vector<Histogram>>
makeHistogramsIfRequired<Data::Histogram>(Dataset &dataset,
                                          const std::string &name) {
  auto histograms = std::make_unique<std::vector<Histogram>>(0);
  histograms->reserve(4);
  const auto edges = dataset.get<const Data::Tof>();
  auto values = dataset.get<Data::Value>(name);
  auto errors = dataset.get<Data::Error>(name);
  histograms->emplace_back(Unit::Id::Length, 2, 1, edges.data(), values.data(),
                           errors.data());
  histograms->emplace_back(Unit::Id::Length, 2, 1, edges.data() + 3,
                           values.data() + 2, errors.data() + 2);
  return histograms;
}

template <class Tag>
auto returnReference(
    Dataset &dataset,
    const std::unique_ptr<std::vector<Histogram>> &histograms) {
  return dataset.get<detail::value_type_t<Tag>>();
}

template <class Tag>
auto returnReference(
    Dataset &dataset, const std::string &name,
    const std::unique_ptr<std::vector<Histogram>> &histograms) {
  return dataset.get<detail::value_type_t<Tag>>(name);
}

template <>
auto returnReference<Data::Histogram>(
    Dataset &dataset,
    const std::unique_ptr<std::vector<Histogram>> &histograms) {
  return gsl::make_span(*histograms);
}

template <>
auto returnReference<Coord::SpectrumPosition>(
    Dataset &dataset,
    const std::unique_ptr<std::vector<Histogram>> &histograms) {
  return ref_type_t<Coord::SpectrumPosition>(
      dataset.get<detail::value_type_t<Coord::DetectorPosition>>(),
      dataset.get<detail::value_type_t<Coord::DetectorGrouping>>());
}

/// Class with overloads used to handle "virtual" variables such as
/// Coord::SpectrumPosition.
template <class Tag>
element_reference_type_t<Tag> itemGetHelper(ref_type_t<Tag> &data,
                                            gsl::index index) {
  return data[index];
}

template <>
element_reference_type_t<Coord::SpectrumPosition>
itemGetHelper<Coord::SpectrumPosition>(
    ref_type_t<Coord::SpectrumPosition> &data, gsl::index index) {
  if (data.second[index].empty())
    throw std::runtime_error("Spectrum has no detectors, cannot get position.");
  double position = 0.0;
  for (const auto det : data.second[index])
    position += data.first[det];
  return position /= data.second[index].size();
}

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
  // TODO detail::index is from variable.h, put it somewhere else and rename.
  template <class Tag>
  static constexpr size_t tag_index = detail::index<Tag, tags>::value;
  template <class Tag>
  using maybe_const = std::tuple_element_t<tag_index<Tag>, std::tuple<Ts...>>;

  Dimensions
  relevantDimensions(const std::vector<Dimensions> &variableDimensions) {
    const auto &largest =
        *std::max_element(variableDimensions.begin(), variableDimensions.end(),
                          [](const Dimensions &a, const Dimensions &b) {
                            return a.count() < b.count();
                          });

    std::vector<bool> is_const{std::is_const<Ts>::value...};
    for (gsl::index i = 0; i < sizeof...(Ts); ++i) {
      const auto &dims = variableDimensions[i];
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

  template <class Tag> auto getData(Dataset &dataset) {
    m_histograms = makeHistogramsIfRequired<Tag>(dataset);
    return returnReference<Tag>(dataset, m_histograms);
  }

  template <class Tag> auto getData(Dataset &dataset, const std::string &name) {
    m_histograms = makeHistogramsIfRequired<Tag>(dataset, name);
    return returnReference<Tag>(dataset, name, m_histograms);
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
    element_reference_type_t<maybe_const<Tag>>
    get(std::enable_if_t<!detail::is_bins<Tag>::value> * = nullptr) const {
      // Should we allow passing const?
      static_assert(!std::is_const<Tag>::value, "Do not use `const` qualifier "
                                                "for tags when accessing "
                                                "DatasetView::iterator.");
      constexpr auto variableIndex = tag_index<Tag>;
      auto &col = std::get<variableIndex>(m_variables);

      // TODO Ensure that this is inlined and does not affect performance.
      return itemGetHelper<
          std::tuple_element_t<variableIndex, std::tuple<Ts...>>>(
          col, m_index.get<variableIndex>());
    }

    template <class Tag>
    element_reference_type_t<maybe_const<Tag>>
    get(std::enable_if_t<detail::is_bins<Tag>::value> * = nullptr) const {
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
      : m_subdimensions{DimensionHelper<Ts>::get(dataset, name,
                                                 fixedDimensions)...},
        m_relevantDimensions(relevantDimensions(m_subdimensions)),
        m_columns(getData<Ts>(dataset, name)...) {}
  DatasetView(Dataset &dataset, const std::set<Dimension> &fixedDimensions = {})
      : m_subdimensions{DimensionHelper<Ts>::get(dataset, fixedDimensions)...},
        m_relevantDimensions(relevantDimensions(m_subdimensions)),
        m_columns(getData<Ts>(dataset)...) {}

  iterator begin() {
    return {0, m_relevantDimensions, m_subdimensions, m_columns};
  }
  iterator end() {
    return {m_relevantDimensions.volume(), m_relevantDimensions,
            m_subdimensions, m_columns};
  }

private:
  const std::vector<Dimensions> m_subdimensions;
  const Dimensions m_relevantDimensions;
  std::unique_ptr<std::vector<Histogram>> m_histograms;
  std::tuple<ref_type_t<Ts>...> m_columns;
};

#endif // DATASET_VIEW_H
