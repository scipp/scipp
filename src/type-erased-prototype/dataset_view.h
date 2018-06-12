#ifndef DATASET_VIEW_H
#define DATASET_VIEW_H

#include <algorithm>
#include <tuple>
#include <type_traits>

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

struct MultidimensionalIndex {
  MultidimensionalIndex() = default;
  MultidimensionalIndex(const std::vector<gsl::index> dimension)
      : index(dimension.size()), dimension(dimension), end(dimension) {
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

  std::vector<gsl::index> index{0};
  std::vector<gsl::index> dimension{0};
  std::vector<gsl::index> end{0};
};

class LinearSubindex {
public:
  LinearSubindex(const std::map<Dimension, gsl::index> &variableDimensions,
                 const Dimensions &dimensions,
                 const MultidimensionalIndex &index)
      : m_index(index) {
    gsl::index factor{1};
    for (gsl::index i = 0; i < dimensions.count(); ++i) {
      const auto dimension = dimensions.label(i);
      if (variableDimensions.count(dimension)) {
        m_offsets.push_back(std::distance(variableDimensions.begin(),
                                          variableDimensions.find(dimension)));
        m_factors.push_back(factor);
      }
      factor *= dimensions.size(i);
    }
  }

  gsl::index get() const {
    gsl::index index{0};
    for (gsl::index i = 0; i < m_factors.size(); ++i)
      index += m_factors[i] * m_index.index[m_offsets[i]];
    return index;
  }

private:
  std::vector<gsl::index> m_factors;
  std::vector<gsl::index> m_offsets;
  const MultidimensionalIndex &m_index;
};

template <class Base, class T> struct GetterMixin {};

template <class Base> struct GetterMixin<Base, Variable::Tof> {
  const element_reference_type_t<Variable::Tof> tof() {
    return static_cast<Base *>(this)->template get<Variable::Tof>();
  }
};

template <class T> using ref_type = variable_type_t<detail::value_type_t<T>> &;

template <class Tag>
Dimensions getDimensions(const Dataset &dataset) {
  return dataset.dimensions<detail::value_type_t<Tag>>();
}

template <>
Dimensions getDimensions<Variable::Histogram>(const Dataset &dataset) {
  auto dims = dataset.dimensions<Variable::Value>();
  dims.erase(Dimension::Tof);
  return dims;
}

template <class Tag>
std::unique_ptr<std::vector<Histogram>>
makeHistogramsIfRequired(Dataset &dataset) {
  return nullptr;
}

template <>
std::unique_ptr<std::vector<Histogram>>
makeHistogramsIfRequired<Variable::Histogram>(Dataset &dataset) {
  auto histograms = std::make_unique<std::vector<Histogram>>(0);
  histograms->reserve(4);
  const auto &edges = dataset.get<const Variable::Tof>();
  auto &values = dataset.get<Variable::Value>();
  auto &errors = dataset.get<Variable::Error>();
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

template <>
ref_type<Variable::Histogram> returnReference<Variable::Histogram>(
    Dataset &dataset,
    const std::unique_ptr<std::vector<Histogram>> &histograms) {
  return *histograms;
}

// pass non-iterated dimensions in constructor?
// Dataset::begin(Dimension::Tof)??
// Dataset::begin(DoNotIterate::Tof)??
template <class... Ts>
class DatasetView : public GetterMixin<DatasetView<Ts...>, Ts>... {
private:
  std::vector<gsl::index>
  extractExtents(const std::map<Dimension, gsl::index> &dimensions,
                 const std::set<Dimension> &fixedDimensions) {
    std::vector<gsl::index> extents;
    for (const auto &item : dimensions) {
      if (fixedDimensions.count(item.first) == 0)
        extents.push_back(item.second);
      else
        extents.push_back(1); // fake extent 1 implies no index incremented in
                              // this dimension.
    }
    return extents;
  }

  std::map<Dimension, gsl::index>
  relevantDimensions(const Dataset &dataset,
                     const std::set<Dimension> &fixedDimensions) {
    std::vector<bool> is_bins{detail::is_bins<Ts>::value...};
    std::vector<Dimensions> variableDimensions{getDimensions<Ts>(dataset)...};
    auto datasetDimensions = sizeof...(Ts) == 1 && !is_bins[0]
                                 ? variableDimensions[0]
                                 : dataset.dimensions();

    std::set<Dimension> dimensions;
    for (gsl::index i = 0; i < sizeof...(Ts); ++i) {
      const auto &thisVariableDimensions = variableDimensions[i];
      for (gsl::index d = 0; d < thisVariableDimensions.count(); ++d) {
        const auto &dimension = thisVariableDimensions.label(d);
        if (fixedDimensions.count(dimension) == 0) {
          if (thisVariableDimensions.size(d) !=
              datasetDimensions.size(dimension) + (is_bins[i] ? 1 : 0))
            throw std::runtime_error(
                "One of the variables requested for iteration represents bin "
                "edges, direct joint iteration is not possible. Use the Bins<> "
                "wrapper to iterate over bins defined by edges instead.");
          dimensions.insert(dimension);
        }
      }
    }

    std::map<Dimension, gsl::index> relevantDimensions;
    for (const auto &dimension : dimensions)
      relevantDimensions[dimension] = datasetDimensions.size(dimension);

    std::vector<bool> is_const{std::is_const<Ts>::value...};
    for (gsl::index i = 0; i < sizeof...(Ts); ++i) {
      if (is_const[i])
        continue;
      const auto &thisVariableDimensions = variableDimensions[i];
      std::vector<Dimension> diff;
      for (gsl::index i = 0; i < thisVariableDimensions.count(); ++i) {
        const auto &dimension = thisVariableDimensions.label(i);
        if (!fixedDimensions.count(dimension))
          diff.push_back(dimension);
      }
      if (dimensions.size() != diff.size())
        throw std::runtime_error("Variables requested for iteration have "
                                 "different dimensions");
    }

    return relevantDimensions;
  }

  template <class Tag> ref_type<Tag> getData(Dataset &dataset) {
    m_histograms = makeHistogramsIfRequired<Tag>(dataset);
    return returnReference<Tag>(dataset, m_histograms);
  }

public:
  DatasetView(Dataset &dataset, const std::set<Dimension> &fixedDimensions = {})
      : m_relevantDimensions(relevantDimensions(dataset, fixedDimensions)),
        m_index(extractExtents(m_relevantDimensions, fixedDimensions)),
        m_columns(
            std::tuple<Ts, LinearSubindex, ref_type<Ts>, detail::is_slab_t<Ts>>(
                Ts{}, LinearSubindex(m_relevantDimensions,
                                     getDimensions<Ts>(dataset), m_index),
                getData<Ts>(dataset), detail::is_slab<Ts>{})...) {}

  // TODO add get version for Slab.
  // TODO const/non-const versions.
  template <class Tag>
  element_reference_type_t<Tag>
  get(std::enable_if_t<!detail::is_bins<Tag>::value> * = nullptr) {
    auto &col = std::get<std::tuple<Tag, LinearSubindex, variable_type_t<Tag> &,
                                    std::false_type>>(m_columns);
    return std::get<2>(col)[std::get<LinearSubindex>(col).get()];
  }

  template <class Tag>
  element_reference_type_t<Tag>
  get(std::enable_if_t<detail::is_bins<Tag>::value> * = nullptr) {
    auto &col = std::get<std::tuple<Tag, LinearSubindex, variable_type_t<Tag> &,
                                    std::false_type>>(m_columns);
    const auto &data = std::get<2>(col);
    const auto index = std::get<LinearSubindex>(col).get();
    // TODO This is wrong if Bins are not the innermost index.
    return Bin(data[index], data[index + 1]);
  }

  // TODO Very bad temporary interface just for testing, make proper begin()
  // and end() methods, etc.
  void increment() { m_index.increment(); }
  bool atLast() { return m_index.index == m_index.end; }

private:
  std::map<Dimension, gsl::index> m_relevantDimensions;
  MultidimensionalIndex m_index;
  // Ts is a dummy used for Tag-based lookup.
  //std::unique_ptr<Histogram[]> m_histograms;
  std::unique_ptr<std::vector<Histogram>> m_histograms;
  std::tuple<std::tuple<Ts, LinearSubindex, ref_type<Ts>,
                        detail::is_slab_t<Ts>>...> m_columns;
};

#endif // DATASET_VIEW_H
