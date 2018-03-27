#ifndef DATASET_ITERATOR_H
#define DATASET_ITERATOR_H

#include <tuple>

#include "dataset.h"

template <class... Ts>
class DatasetItem {
  public:

  private:

};

template <class... Ts>
class DatasetIterator {
  public:
    DatasetIterator(Dataset &dataset, const gsl::index index = 0)
        : m_index(index),
          m_columns(std::forward_as_tuple(
              dataset.get<std::vector<std::remove_const_t<Ts>>>()...)) {}

    template <class T> T &get() {
      // Works as long as all columns share the same dimensions.
      // Where should we fail if T has extra dimensions? Default iterator: iterate everything (exception: TOF if requested). If not iterating everything: must request slice of columns
      return std::get<std::vector<T> &>(m_columns)[m_index];
    }

    // Increment each dimension separately?
    // May not want to iterate all (in particular TOF!). Can we request that by having Histogram (or similar) as requested type, instead of item type? That is, Dataset could contain std::vector<BinEdge>, std::vector<Value>, std::vector<Error> but we ask for ther plural versions, and thus make it skip that iteration?
    // We know only at runtime which dimensions contribute to each column. How do we set this up at runtime?
    // - iterate dimensions in certain order, incrementing index with dimensions
    // - for each column, go though list of its dimension and compute index based on above set of indices
    // Is this very expensive? Is that an issue if we do not iterate the innermost (TOF) index?
    void increment() {
      ++m_index;
#if 0
      // full multi dimensional version:
      // unknown size since arbitrary dimensions possible (but could be put on stack since it is always small):
      std::vector<gsl::index> index;
      std::array<std::vector<gsl::index>, sizeof...(Ts)> columnDimensions;
      // 1. Increment index as a normal multi-dimensional index.
      // 2. compute column index:
      std::array<gsl::index, sizeof...(Ts)> columnIndices;
      for (auto i : columnIndices) {
        i = 0;
        gsl::index prev_dimension = -1;
        for (const auto dimension : columnDimensions) {
          if (prev_dimension >= 0)
            i *= m_dimensions[prev_dimension];
          prev_dimensions = dimension;
          i += index[dimension];
        }
      }
#endif
    }

  private:
    gsl::index m_index;
    std::tuple<std::vector<Ts> &...> m_columns;
};

#endif // DATASET_ITERATOR_H
