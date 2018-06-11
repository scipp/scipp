#ifndef DATASET_ITERATOR_H
#define DATASET_ITERATOR_H

#include <algorithm>
#include <tuple>
#include <type_traits>

#include "dataset.h"

template <class T> struct Slab { using value_type = T; };

namespace detail {
template <class T> struct value_type { using type = T; };
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

// pass non-iterated dimensions in constructor?
// Dataset::begin(Dimension::Tof)??
// Dataset::begin(DoNotIterate::Tof)??
template <class... Ts>
class DatasetIterator : public GetterMixin<DatasetIterator<Ts...>, Ts>... {
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
    std::vector<Dimensions> variableDimensions{
        dataset.dimensions<detail::value_type_t<Ts>>()...};
    auto datasetDimensions =
        sizeof...(Ts) == 1 ? variableDimensions[0] : dataset.dimensions();

    std::set<Dimension> dimensions;
    for (const auto &thisVariableDimensions : variableDimensions) {
      for (gsl::index i = 0; i < thisVariableDimensions.count(); ++i) {
        const auto &dimension = thisVariableDimensions.label(i);
        if (fixedDimensions.count(dimension) == 0) {
          if (thisVariableDimensions.size(i) !=
              datasetDimensions.size(dimension))
            throw std::runtime_error("");
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

public:
  template <class T>
  using ref_type = variable_type_t<detail::value_type_t<T>> &;
  DatasetIterator(Dataset &dataset,
                  const std::set<Dimension> &fixedDimensions = {})
      : m_relevantDimensions(relevantDimensions(dataset, fixedDimensions)),
        m_index(extractExtents(m_relevantDimensions, fixedDimensions)),
        m_columns(
            std::tuple<LinearSubindex, ref_type<Ts>, detail::is_slab_t<Ts>>(
                LinearSubindex(m_relevantDimensions,
                               dataset.dimensions<detail::value_type_t<Ts>>(),
                               m_index),
                dataset.get<detail::value_type_t<Ts>>(),
                detail::is_slab<Ts>{})...) {}

  // TODO create special named getters for frequently used types such as value
  // and errors.
  // get<double> would fail in that case because both value and error are of
  // type double.
  // TODO add get version for Slab.
  // TODO const/non-const versions.
  template <class Tag> element_reference_type_t<Tag> get() {
    auto &col = std::get<
        std::tuple<LinearSubindex, variable_type_t<Tag> &, std::false_type>>(
        m_columns);
    return std::get<1>(col)[std::get<LinearSubindex>(col).get()];
  }

  // TODO Very bad temporary interface just for testing, make proper begin()
  // and end() methods, etc. (rename to DatasetView?).
  void increment() { m_index.increment(); }
  bool atLast() { return m_index.index == m_index.end; }

private:
  std::map<Dimension, gsl::index> m_relevantDimensions;
  MultidimensionalIndex m_index;
  std::tuple<std::tuple<LinearSubindex, ref_type<Ts>, detail::is_slab_t<Ts>>...>
      m_columns;
};

#endif // DATASET_ITERATOR_H
