// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <ostream>

#include "scipp/core/dtype.h"
#include "scipp/core/eigen.h"
#include "scipp/core/except.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/core/string.h"

namespace scipp::core {

bool is_int(DType tp) { return tp == dtype<int32_t> || tp == dtype<int64_t>; }

bool is_float(DType tp) { return tp == dtype<float> || tp == dtype<double>; }

bool is_fundamental(DType tp) {
  return is_int(tp) || is_float(tp) || tp == dtype<bool>;
}

bool is_total_orderable(DType tp) {
  return is_fundamental(tp) || tp == dtype<time_point>;
}

namespace {
template <class... Ts> bool is_span_impl(DType tp) {
  return (
      ((tp == dtype<std::span<Ts>>) || (tp == dtype<std::span<const Ts>>)) ||
      ...);
}
} // namespace

bool is_span(DType tp) {
  return is_span_impl<double, float, int64_t, int32_t, bool, time_point>(tp);
}

std::ostream &operator<<(std::ostream &os, const DType &dtype) {
  return os << to_string(dtype);
}

auto register_dtype_name_void(
    (core::dtypeNameRegistry().emplace(dtype<void>, "void"), 0));

namespace {
template <class T, class... Ts> constexpr auto inner_lut() {
  return std::array{std::pair{dtype<Ts>, dtype<std::common_type_t<T, Ts>>}...};
}

template <class... Ts> constexpr auto outer_lut() {
  return std::array{std::pair{dtype<Ts>, inner_lut<Ts, Ts...>()}...};
}
} // namespace

DType common_type(const DType &a, const DType &b) {
  if (a == b)
    return a;
  const auto lut = outer_lut<double, float, int64_t, int32_t>();
  const auto it = std::find_if(lut.begin(), lut.end(), [&](const auto &pair) {
    return pair.first == a;
  });
  if (it == lut.end())
    throw except::TypeError("'common_type' does not support dtype " +
                            to_string(a));
  const auto it2 =
      std::find_if(it->second.begin(), it->second.end(),
                   [&](const auto &pair) { return pair.first == b; });
  if (it2 == it->second.end())
    throw except::TypeError("'common_type' does not support dtype " +
                            to_string(b));
  return it2->second;
}

} // namespace scipp::core
