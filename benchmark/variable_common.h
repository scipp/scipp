#pragma once

#include <random>

#include "../variable/test/make_sparse.h"

template <typename T> struct GenerateSparse {
  auto operator()(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);

    auto a = make_sparse_variable<T>(length);
    unsigned long long size(0);

    /* Generate a random amount of sparse data for each point */
    auto vals = a.template values<event_list<T>>();
    for (scipp::index i = 0; i < length; ++i) {
      const auto l = dis(gen);
      size += l;
      vals[i] = scipp::core::sparse_container<T>(l, i);
    }

    return std::make_tuple(a, sizeof(T) * size);
  }
};
