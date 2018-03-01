#ifndef DATA_FRAME_VIEW_H
#define DATA_FRAME_VIEW_H

#include <tuple>

#include "data_frame.h"

// Problem: We have a DataFrame, something like:
// std::tuple<std::vector<int>, std::vector<double>>;
// Want to access it as if it was (optionally with const)
// std::tuple<std::vector<double>>

template <class... Ts, class Tuple>
std::tuple<Ts &...> makeTupleView(Tuple &data) {
  return std::forward_as_tuple(std::get<std::remove_const_t<Ts>>(data)...);
}

// Doing this in a recursive manner might be difficult, but we can just do it
// step-by-step in the call wrapper?

#endif // DATA_FRAME_VIEW_H
