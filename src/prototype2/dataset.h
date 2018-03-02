#ifndef DATASET_H
#define DATASET_H

#include <array>
#include <vector>

namespace gsl {
using index = ptrdiff_t;
}

// Inspired by xarray.Dataset

// std::pair<std::vector<std::string>>, std::vector<std::vector<double>>>
// data1({"x", "y"}, {});
// std::pair<std::vector<std::string>>, std::vector<double>> data2({"x"}, {});
// std::pair<std::vector<std::string>>, std::vector<double>> data3({"y"}, {});

// begin("x") -> iterate x, data1 is vector (potentially with stride), data2 is
// double, data 3 is fixed double

// need two cases: axis is bin edges, axis is points

enum class Dimension { SpectrumNumber, Run, DetectorId, Tof, Q };

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

  // how to get iterator centered to a certain type?
  template <class T> const auto &at(const gsl::index i) const {
    // data items fall in three cases:
    // 1. dimensions match those of T => pass reference
    // 2. misses dimension(s) of T => pass const reference
    // 3. has additional dimensions => pass reference to container with stride access
  }

private:
  std::map<std::string, gsl::index> m_dimensions;
  std::tuple<std::pair<std::vector<std::string>, std::vector<Ts>>...> m_data;
};

template <class... Ts> class Dataset {
public:
  Dataset(std::array<std::vector<Dimension>, sizeof...(Ts)> dimensions,
          Ts &&... data)
      : m_dimensions(std::move(dimensions)), m_data(std::forward<Ts>(data)...) {
    // TODO check that data items sharing dimensions have same length in that
    // dimension.
  }

  /*
  template <class T> const T &get() const {
    return std::get<T>(m_data);
  }

  template <int MaxSize> gsl::index getSize(const gsl::index i) const;

  template <>
  gsl::index getSize<0>(const gsl::index i) const {
    return -1;
  }

  template <>
  gsl::index getSize<1>(const gsl::index i) const {
    switch (i) {
    case 0:
      return std::get<0>(m_data).size();
    }
    return -1;
  }

  template <>
  gsl::index getSize<2>(const gsl::index i) const {
    switch (i) {
    case 0:
      return std::get<0>(m_data).size();
    case 1:
      return std::get<1>(m_data).size();
    }
    return -1;
  }
  */

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

  // template <Dimension Dim>
  //  auto operator[](const gsl::index i)
  //  return tuple of references? (dedicated item later)
  //  {}

private:
  // Dimensions for each of the data items.
  std::array<std::vector<Dimension>, sizeof...(Ts)> m_dimensions;
  std::tuple<Ts...> m_data;
};

#endif // DATASET_H
