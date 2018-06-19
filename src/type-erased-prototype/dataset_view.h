#ifndef DATASET_VIEW_H
#define DATASET_VIEW_H

#include <algorithm>
#include <tuple>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>

#include "dataset.h"
#include "histogram.h"

template <class T> struct Slab { using value_type = T; };

namespace detail {
template <class T> struct value_type { using type = T; };
template <class T> struct value_type<Bins<T>> { using type = T; };
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
template <class T> struct is_bins<Bins<T>> : public std::true_type {};
template <class T> using is_bins_t = typename is_bins<T>::type;
}

class MultidimensionalIndex {
private:
  std::vector<gsl::index> extractExtents(const Dimensions &dimensions) {
    std::vector<gsl::index> extents;
    for (const auto &item : dimensions)
      extents.push_back(item.second);
    return extents;
  }

public:
  MultidimensionalIndex() = default;
  MultidimensionalIndex(const Dimensions &dimensions)
      : dimensions(dimensions), index(dimensions.count()),
        dimension(extractExtents(dimensions)), end(dimension) {
    for (auto &n : end)
      --n;
  }

  void increment() {
    ++index[0];
    for (gsl::index i = 0; i < index.size(); ++i) {
      if (index[i] == dimension[i]) {
        index[i] = 0;
        ++index[i + 1];
      } else {
        break;
      }
    }
  }

  const Dimensions dimensions;
  std::vector<gsl::index> index{0};
  std::vector<gsl::index> dimension{0};
  std::vector<gsl::index> end{0};
};

class LinearSubindex {
public:
  LinearSubindex(const Dimensions &parentDimensions,
                 const Dimensions &dimensions) {
    gsl::index factor{1};
    for (gsl::index i = 0; i < dimensions.count(); ++i) {
      const auto dimension = dimensions.label(i);
      if (parentDimensions.contains(dimension)) {
        m_offsets.push_back(parentDimensions.index(dimension));
        m_factors.push_back(factor);
      }
      factor *= dimensions.size(i);
    }
  }

  gsl::index get(const std::vector<gsl::index> &indices) const {
    gsl::index index{0};
    for (gsl::index i = 0; i < m_factors.size(); ++i)
      index += m_factors[i] * indices[m_offsets[i]];
    return index;
  }

private:
  std::vector<gsl::index> m_factors;
  std::vector<gsl::index> m_offsets;
};

template <class Base, class T> struct GetterMixin {};

template <class Base> struct GetterMixin<Base, Data::Tof> {
  const element_reference_type_t<Data::Tof> tof() {
    return static_cast<Base *>(this)->template get<Data::Tof>();
  }
};

template <class Base> struct GetterMixin<Base, Data::Value> {
  const element_reference_type_t<Data::Value> value() {
    return static_cast<Base *>(this)->template get<Data::Value>();
  }
};

template <class T> using ref_type = variable_type_t<detail::value_type_t<T>> &;

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

template <class Tag> struct DimensionHelper<Bins<Tag>> {
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
  const auto &edges = dataset.get<const Data::Tof>();
  auto &values = dataset.get<Data::Value>();
  auto &errors = dataset.get<Data::Error>();
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
  const auto &edges = dataset.get<const Data::Tof>();
  auto &values = dataset.get<Data::Value>(name);
  auto &errors = dataset.get<Data::Error>(name);
  histograms->emplace_back(Unit::Id::Length, 2, 1, edges.data(), values.data(),
                           errors.data());
  histograms->emplace_back(Unit::Id::Length, 2, 1, edges.data() + 3,
                           values.data() + 2, errors.data() + 2);
  return histograms;
}

template <class Tag>
ref_type<Tag>
returnReference(Dataset &dataset,
                const std::unique_ptr<std::vector<Histogram>> &histograms) {
  return dataset.get<detail::value_type_t<Tag>>();
}

template <class Tag>
ref_type<Tag>
returnReference(Dataset &dataset, const std::string &name,
                const std::unique_ptr<std::vector<Histogram>> &histograms) {
  return dataset.get<detail::value_type_t<Tag>>(name);
}

template <>
ref_type<Data::Histogram> returnReference<Data::Histogram>(
    Dataset &dataset,
    const std::unique_ptr<std::vector<Histogram>> &histograms) {
  return *histograms;
}

// pass non-iterated dimensions in constructor?
// Dataset::begin(Dimension::Tof)??
// Dataset::begin(DoNotIterate::Tof)??
template <class... Ts>
class DatasetView : public GetterMixin<DatasetView<Ts...>, Ts>... {
  static_assert(sizeof...(Ts),
                "DatasetView requires at least one variable for iteration");

private:
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
            "iteration is not possible. Use the Bins<> "
            "wrapper to iterate over bins defined by edges instead.");
      // Must either be identical or access must be read-only.
      if (!((largest == dims) || is_const[i]))
        throw std::runtime_error("Variables requested for iteration have "
                                 "different dimensions");
    }
    return largest;
  }

  template <class Tag> ref_type<Tag> getData(Dataset &dataset) {
    m_histograms = makeHistogramsIfRequired<Tag>(dataset);
    return returnReference<Tag>(dataset, m_histograms);
  }

  template <class Tag>
  ref_type<Tag> getData(Dataset &dataset, const std::string &name) {
    m_histograms = makeHistogramsIfRequired<Tag>(dataset, name);
    return returnReference<Tag>(dataset, name, m_histograms);
  }

public:
  class iterator;
  class Item {
  public:
    Item(const gsl::index index, const std::vector<gsl::index> &dimensions,
         std::tuple<std::tuple<Ts, LinearSubindex, ref_type<Ts>>...> &variables)
        : m_indices(dimensions.size()), m_dimensions(dimensions),
          m_variables(variables) {
      setIndex(index);
    }

    void setIndex(const gsl::index index) {
      if (m_dimensions.empty())
        return;
      auto remainder{index};
      for (gsl::index d = 0; d < m_dimensions.size()-1; ++d) {
        m_indices[d] = remainder % m_dimensions[d];
        remainder /= m_dimensions[d];
      }
      m_indices.back() = remainder;
    }

    template <class Tag>
    element_reference_type_t<Tag>
    get(std::enable_if_t<!detail::is_bins<Tag>::value> * = nullptr) const {
      auto &col =
          std::get<std::tuple<Tag, LinearSubindex, variable_type_t<Tag> &>>(
              m_variables);
      return std::get<2>(col)[std::get<LinearSubindex>(col).get(m_indices)];
    }

    bool operator==(const Item &other) const {
      if (m_indices != other.m_indices)
        return false;
      if (&m_dimensions != &other.m_dimensions)
        return false;
      if (&m_variables != &other.m_variables)
        return false;
      return true;
    }

  private:
    friend class iterator;
    Item(const Item &other) = default;
    std::vector<gsl::index> m_indices;
    const std::vector<gsl::index> &m_dimensions;
    std::tuple<std::tuple<Ts, LinearSubindex, ref_type<Ts>>...> &m_variables;
  };

  class iterator
      : public boost::iterator_facade<iterator, const Item,
                                      boost::random_access_traversal_tag> {
  public:
    iterator(
        const gsl::index index, const std::vector<gsl::index> &dimensions,
        std::tuple<std::tuple<Ts, LinearSubindex, ref_type<Ts>>...> &variables)
        : m_index(index), m_item(index, dimensions, variables) {}

  private:
    friend class boost::iterator_core_access;

    bool equal(const iterator &other) const {
      return m_index == other.m_index && m_item == other.m_item;
    }

    void increment() {
      ++m_index;
      m_item.setIndex(m_index);
    }

    const Item &dereference() const { return m_item; }

    void decrement() {
      --m_index;
      m_item.setIndex(m_index);
    }
    void advance(int64_t delta) {
      m_index += delta;
      m_item.setIndex(m_index);
    }

    int64_t distance_to(const iterator &other) const {
      return static_cast<int64_t>(other.m_index) -
             static_cast<int64_t>(m_index);
    }

    gsl::index m_index;
    Item m_item;
  };

  DatasetView(Dataset &dataset, const std::string &name,
              const std::set<Dimension> &fixedDimensions = {})
      : m_index(relevantDimensions(
            {DimensionHelper<Ts>::get(dataset, name, fixedDimensions)...})),
        m_columns(std::tuple<Ts, LinearSubindex, ref_type<Ts>>(
            Ts{}, LinearSubindex(
                      m_index.dimensions,
                      DimensionHelper<Ts>::get(dataset, name, fixedDimensions)),
            getData<Ts>(dataset, name))...) {}
  DatasetView(Dataset &dataset, const std::set<Dimension> &fixedDimensions = {})
      : m_index(relevantDimensions(
            {DimensionHelper<Ts>::get(dataset, fixedDimensions)...})),
        m_columns(std::tuple<Ts, LinearSubindex, ref_type<Ts>>(
            Ts{},
            LinearSubindex(m_index.dimensions,
                           DimensionHelper<Ts>::get(dataset, fixedDimensions)),
            getData<Ts>(dataset))...) {}

  iterator begin() { return {0, m_index.dimension, m_columns}; }
  iterator end() {
    return {m_index.dimensions.volume(), m_index.dimension, m_columns};
  }

  // TODO add get version for Slab.
  // TODO const/non-const versions.
  template <class Tag>
  element_reference_type_t<Tag>
  get(std::enable_if_t<!detail::is_bins<Tag>::value> * = nullptr) {
    auto &col =
        std::get<std::tuple<Tag, LinearSubindex, variable_type_t<Tag> &>>(
            m_columns);
    return std::get<2>(col)[std::get<LinearSubindex>(col).get(m_index.index)];
  }

  template <class Tag>
  element_reference_type_t<Tag>
  get(std::enable_if_t<detail::is_bins<Tag>::value> * = nullptr) {
    auto &col =
        std::get<std::tuple<Tag, LinearSubindex, variable_type_t<Tag> &>>(
            m_columns);
    const auto &data = std::get<2>(col);
    const auto index = std::get<LinearSubindex>(col).get(m_index.index);
    // TODO This is wrong if Bins are not the innermost index.
    return Bin(data[index], data[index + 1]);
  }

  // TODO Very bad temporary interface just for testing, make proper begin()
  // and end() methods, etc.
  void increment() { m_index.increment(); }
  bool atLast() { return m_index.index == m_index.end; }

private:
  MultidimensionalIndex m_index;
  std::unique_ptr<std::vector<Histogram>> m_histograms;
  // Ts is a dummy used for Tag-based lookup.
  std::tuple<std::tuple<Ts, LinearSubindex, ref_type<Ts>>...> m_columns;
};

#endif // DATASET_VIEW_H
