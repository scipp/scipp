#ifndef DATASET_ITERATOR_H
#define DATASET_ITERATOR_H

#include <tuple>

#include "dataset.h"

template <class T> struct Slab { using value_type = T; };

namespace detail {
template <class T> struct value_type { using type = T; };
template <class T> struct value_type<Slab<T>> { using type = typename Slab<T>::value_type; };
template <class T> using value_type_t = typename value_type<T>::type;
}

struct MultidimensionalIndex {
  MultidimensionalIndex(const std::vector<gsl::index> dimension)
      : index(dimension.size()), dimension(dimension) {}
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
  std::vector<gsl::index> index;
  std::vector<gsl::index> dimension;
};

class LinearSubindex {
public:
  LinearSubindex(const std::map<Dimension, gsl::index> &dimensionExtents,
                 const std::vector<Dimension> &dimensions,
                 const MultidimensionalIndex &index)
      : m_index(index) {
    gsl::index factor{1};
    for (const auto dimension : dimensions) {
      m_offsets.push_back(std::distance(dimensionExtents.begin(),
                                        dimensionExtents.find(dimension)));
      m_factors.push_back(factor);
      factor *= dimensionExtents.at(dimension);
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

// pass non-iterated dimensions in constructor?
// Dataset::begin(Dimension::Tof)?? 
// Dataset::begin(DoNotIterate::Tof)?? 
template <class... Ts>
class DatasetIterator {
  private:
    std::vector<gsl::index>
    extractExtents(const std::map<Dimension, gsl::index> &dimensions) {
      std::vector<gsl::index> extents;
      for (const auto &item : dimensions)
        extents.push_back(item.second);
      return extents;
    }

  public:
    // TODO do we need forward_as_tuple?
    DatasetIterator(Dataset &dataset)
        : m_index(extractExtents(dataset.dimensions())),
          m_columns(std::pair<LinearSubindex,
                              std::vector<detail::value_type_t<Ts>> &>(
              LinearSubindex(
                  dataset.dimensions(),
                  dataset.dimensions<std::vector<
                      detail::value_type_t<std::remove_const_t<Ts>>>>(),
                  m_index),
              dataset.get<std::vector<
                  detail::value_type_t<std::remove_const_t<Ts>>>>())...) {}

    // TODO create special named getters for frequently used types such as value and errors.
    // get<double> would fail in that case because both value and error are of type double.
    template <class T> T &get() {
      // Works as long as all columns share the same dimensions.
      // Where should we fail if T has extra dimensions? Default iterator: iterate everything (exception: TOF if requested). If not iterating everything: must request slice of columns
      auto &col =
          std::get<std::pair<LinearSubindex, std::vector<T> &>>(m_columns);
      return col.second[col.first.get()];
    }

    void increment() { m_index.increment(); }

  private:
    MultidimensionalIndex m_index;
    std::tuple<std::pair<LinearSubindex,
                         std::vector<detail::value_type_t<Ts>> &>...> m_columns;
};

#endif // DATASET_ITERATOR_H
