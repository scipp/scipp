#ifndef DATASET_H
#define DATASET_H

#include <array>
#include <vector>

namespace gsl {
using index = ptrdiff_t;
}

// need two cases: axis is bin edges, axis is points

enum class Dimension { SpectrumNumber, Run, DetectorId, Tof, Q };

template <class T, class... Ts> class FlatDatasetItem;

template <class... Ts> class FlatDataset {
public:
  FlatDataset()
      : m_data(std::make_tuple(std::make_pair(std::vector<std::string>{},
                                              std::vector<Ts>(1))...)) {}
  // need:
  // - dimensions
  // - axis lengths
  // - list of applicable dimensions for all Ts
  void addDimension(const std::string &name, const gsl::index size) {
    // TODO throw if exists
    m_dimensions[name] = size;
  }

  template <class T> void extendAlongDimension(const std::string &name) {
    auto &data =
        std::get<std::pair<std::vector<std::string>, std::vector<T>>>(m_data);
    data.first.push_back(name);
    data.second.resize(data.second.size() * m_dimensions.at(name));
    // TODO duplicate from slice 0 to all others.
  }

  template <class T> const std::vector<T> &get() const {
    return std::get<std::pair<std::vector<std::string>, std::vector<T>>>(m_data).second;
  }
  template <class T> std::vector<T> &get() {
    return std::get<std::pair<std::vector<std::string>, std::vector<T>>>(m_data).second;
  }

  // how to get iterator centered to a certain type?
  template <class T> auto at(const gsl::index i) {
    return FlatDatasetItem<T, Ts...>(i, *this);
    // data items fall in three cases:
    // 1. dimensions match those of T => pass reference
    // 2. misses dimension(s) of T => pass const reference
    // 3. has additional dimensions => pass reference to container with stride access
    // Problem: Dimensions known only at runtime
    // - Always pass const reference to container with stride access, except T
    //   which can be non-const?
    // - Implies that all fields in returned item are wrapped in vector-like :(
    // - Implicitly convert vector-like to item if size is 1?
  }

  template <class T> const std::vector<std::string> &dimensions() const {
    return std::get<std::pair<std::vector<std::string>, std::vector<T>>>(m_data)
        .first;
  }

  gsl::index size(const std::string &dimension) const {
    return m_dimensions.at(dimension);
  }

private:
  std::map<std::string, gsl::index> m_dimensions;
  std::tuple<std::pair<std::vector<std::string>, std::vector<Ts>>...> m_data;
};

template <class T, class... Ts> class FlatDatasetItem {
public:
  FlatDatasetItem(
      const gsl::index index,
      FlatDataset<Ts...> &data)
      : m_index(index), m_data(data) {}

  T &get() { return m_data.get<T>()[m_index]; }

  template <class U> const U &get() const {
    if (m_data.template dimensions<U>() == m_data.template dimensions<T>())
      return m_data.get<U>()[m_index];
    // Simplest case of dimension mismatch: Dimensionless data such as Logs.
    if (m_data.template dimensions<U>().size() == 0)
      return m_data.get<U>()[0];
    throw std::runtime_error("TODO");
    // if dimensions of U match those of T
    // return get<U>(m_data)[m_index];
    // if U misses dimension of U (hard if dimension *order* is not same?)
    // x + Nx*(y + Ny*z)
    // x + Nx*z
    // return get<U>(m_data)[remove_dimension(m_index, missing_dims)];
    // if U has extra dimension
    // throw -> use different getter return vector-like with stride access?

    // TODO
    // can we afford to do this check for every item? might be expensive. Can
    // it be done once in iterator construction? Probably yes, but is there a
    // way to avoid the cost in indexed access?
    // Do indexed access via a view? Setup things in view construction!
  }

private:
  gsl::index m_index;
  FlatDataset<Ts...> &m_data;
};

template <class... Ts> class Dataset {
public:
  Dataset(std::array<std::vector<Dimension>, sizeof...(Ts)> dimensions,
          Ts &&... data)
      : m_dimensions(std::move(dimensions)), m_data(std::forward<Ts>(data)...) {
    // TODO check that data items sharing dimensions have same length in that
    // dimension.
  }

  gsl::index size(const Dimension dimension) {
    // TODO need to do this with metaprogramming
    // - check all data items, check that they match
    for (gsl::index i = 0; i < m_dimensions.size(); ++i)
      for (gsl::index dim = 0; dim < m_dimensions[i].size(); ++dim)
        if (m_dimensions[i][dim] == dimension) {
          if (dim == 0) {
            switch (i) {
            case 0:
              return std::get<0>(m_data).size();
            case 1:
              return std::get<1>(m_data).size();
            }
          }
        }
    throw std::runtime_error("Dimension not found");
  }

private:
  // Dimensions for each of the data items.
  std::array<std::vector<Dimension>, sizeof...(Ts)> m_dimensions;
  std::tuple<Ts...> m_data;
};

#endif // DATASET_H
