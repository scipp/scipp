#pragma once

#include <random>

#include "../core/test/make_sparse.h"

template <typename T> struct GenerateSparse {
  Variable operator()(int size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);

    auto a = make_sparse_variable<T>(size);

    /* Generate a random amount of sparse data for each point */
    auto vals = a.template sparseValues<T>();
    for (scipp::index i = 0; i < size; ++i)
      vals[i] = scipp::core::sparse_container<T>(dis(gen), i);

    return a;
  }
};

template <int NameLen> struct Generate3DWithDataItems {
  auto operator()(const int itemCount = 5, const int size = 100) {
    Dataset d;
    for (auto i = 0; i < itemCount; ++i) {
      d.setData(std::to_string(i) + std::string(NameLen, 'i'),
                makeVariable<double>(
                    {{Dim::X, size}, {Dim::Y, size}, {Dim::Z, size}}));
    }
    return d;
  }
};

template <int NameLen> struct GenerateWithSparseDataItems {
  Dataset operator()(const int itemCount = 5, const int size = 100) {
    Dataset d;
    GenerateSparse<double> gen;
    for (auto i = 0; i < itemCount; ++i) {
      d.setData(std::to_string(i) + std::string(NameLen, 'i'), gen(size));
    }
    return d;
  }
};
