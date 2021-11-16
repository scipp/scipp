#pragma once

#include <random>

#include "scipp/variable/bins.h"

using namespace scipp;

template <typename T> struct GenerateEvents {
  auto operator()(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<scipp::index> dis(0, 100);

    Dimensions dims{Dim::Y, length};
    Variable indices =
        makeVariable<std::pair<scipp::index, scipp::index>>(dims);

    scipp::index size(0);
    auto vals = indices.values<std::pair<scipp::index, scipp::index>>();
    for (scipp::index i = 0; i < length; ++i) {
      const auto l = dis(gen);
      // cppcheck-suppress unreadVariable  # Read through `indices`.
      vals[i] = {size, size + l};
      size += l;
    }
    Variable buffer = makeVariable<T>(Dims{Dim::Event}, Shape{size});

    return std::make_tuple(make_bins(indices, Dim::Event, buffer),
                           sizeof(T) * size);
  }
};
