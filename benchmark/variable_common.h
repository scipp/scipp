#pragma once

#include <random>

#include "../variable/test/make_events.h"

template <typename T> struct GenerateEvents {
  auto operator()(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);

    auto a = make_events_variable<T>(length);
    unsigned long long size(0);

    /* Generate a random amount of event data for each point */
    auto vals = a.template values<event_list<T>>();
    for (scipp::index i = 0; i < length; ++i) {
      const auto l = dis(gen);
      size += l;
      vals[i] = scipp::core::event_list<T>(l, i);
    }

    return std::make_tuple(a, sizeof(T) * size);
  }
};
