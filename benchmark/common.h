#pragma once

#include "scipp/dataset/dataset.h"

#include "variable_common.h"

template <int NameLen> struct Generate3DWithDataItems {
  auto operator()(const int itemCount = 5, const int length = 100) {
    Dataset d;
    for (auto i = 0; i < itemCount; ++i) {
      d.setData(std::to_string(i) + std::string(NameLen, 'i'),
                makeVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z},
                                     Shape{length, length, length}));
    }
    return std::make_tuple(d, sizeof(double) * itemCount * std::pow(length, 3));
  }
};

template <int NameLen> struct GenerateWithEventsDataItems {
  auto operator()(const int itemCount = 5, const int length = 100) {
    Dataset d;
    GenerateEvents<double> gen;
    unsigned long long size(0);
    for (auto i = 0; i < itemCount; ++i) {
      const auto [data, s] = gen(length);
      size += s;
      d.setData(std::to_string(i) + std::string(NameLen, 'i'), data);
    }
    return std::make_tuple(d, size);
  }
};
