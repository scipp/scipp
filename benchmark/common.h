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
