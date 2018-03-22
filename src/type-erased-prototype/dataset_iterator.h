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
      // Works as long as all columsn share the same dimensions.
      return std::get<std::vector<T> &>(m_columns)[m_index];
    }

    void increment() { ++m_index; }

  private:
    gsl::index m_index;
    std::tuple<std::vector<Ts> &...> m_columns;
};

#endif // DATASET_ITERATOR_H
