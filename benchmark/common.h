#pragma once

#include <random>

#include "../core/test/make_sparse.h"

template <typename T> struct GenerateSparse {
  auto operator()(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);

    auto a = make_sparse_variable<T>(length);
    unsigned long long size(0);

    /* Generate a random amount of sparse data for each point */
    auto vals = a.template sparseValues<T>();
    for (scipp::index i = 0; i < length; ++i) {
      const auto l = dis(gen);
      size += l;
      vals[i] = scipp::core::sparse_container<T>(l, i);
    }

    return std::make_tuple(a, sizeof(T) * size);
  }
};

template <int NameLen> struct Generate3DWithDataItems {
  auto operator()(const int itemCount = 5, const int length = 100) {
    Dataset d;
    for (auto i = 0; i < itemCount; ++i) {
      d.setData(std::to_string(i) + std::string(NameLen, 'i'),
                createVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z},
                                       Shape{length, length, length}));
    }
    return std::make_tuple(d, sizeof(double) * itemCount * std::pow(length, 3));
  }
};

template <int NameLen> struct GenerateWithSparseDataItems {
  auto operator()(const int itemCount = 5, const int length = 100) {
    Dataset d;
    GenerateSparse<double> gen;
    unsigned long long size(0);
    for (auto i = 0; i < itemCount; ++i) {
      const auto [data, s] = gen(length);
      size += s;
      d.setData(std::to_string(i) + std::string(NameLen, 'i'), data);
    }
    return std::make_tuple(d, size);
  }
};
