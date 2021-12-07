// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <functional>
#include <unordered_map>

#include "scipp-core_export.h"
#include "scipp/common/span.h"
#include "scipp/core/time_point.h"

namespace scipp::core {

struct SCIPP_CORE_EXPORT DType {
  int32_t index;
  constexpr bool operator==(const DType &t) const noexcept {
    return index == t.index;
  }
  constexpr bool operator!=(const DType &t) const noexcept {
    return index != t.index;
  }
  constexpr bool operator<(const DType &t) const noexcept {
    return index < t.index;
  }
};

// Note that previously we where using std::type_info to obtain a unique ID,
// however this was causing trouble across library boundaries on certain systems
// (macOS). This mechanism based on hand-code IDs has the risk of clashing dtype
// for two different types T1 and T2, but we currently do not have a better
// solution.
// force compiler error if not specialized
template <class T> constexpr DType dtype{T::missing_specialization_of_dtype};
// basics types start at 0
template <> inline constexpr DType dtype<void>{0};
template <> inline constexpr DType dtype<double>{1};
template <> inline constexpr DType dtype<float>{2};
template <> inline constexpr DType dtype<int64_t>{3};
template <> inline constexpr DType dtype<int32_t>{4};
template <> inline constexpr DType dtype<bool>{5};
template <> inline constexpr DType dtype<std::string>{6};
template <> inline constexpr DType dtype<time_point>{7};
class SubbinSizes;
template <> inline constexpr DType dtype<SubbinSizes>{10};
// span<T> start at 100
template <> inline constexpr DType dtype<scipp::span<const double>>{100};
template <> inline constexpr DType dtype<scipp::span<const float>>{101};
template <> inline constexpr DType dtype<scipp::span<const int64_t>>{102};
template <> inline constexpr DType dtype<scipp::span<const int32_t>>{103};
template <> inline constexpr DType dtype<scipp::span<const bool>>{104};
template <> inline constexpr DType dtype<scipp::span<const std::string>>{105};
template <> inline constexpr DType dtype<scipp::span<const time_point>>{106};
// span<inline const T> start at 200
template <> inline constexpr DType dtype<scipp::span<double>>{200};
template <> inline constexpr DType dtype<scipp::span<float>>{201};
template <> inline constexpr DType dtype<scipp::span<int64_t>>{202};
template <> inline constexpr DType dtype<scipp::span<int32_t>>{203};
template <> inline constexpr DType dtype<scipp::span<bool>>{204};
template <> inline constexpr DType dtype<scipp::span<std::string>>{205};
template <> inline constexpr DType dtype<scipp::span<time_point>>{206};
// std containers start at 300
template <> inline constexpr DType dtype<std::pair<int32_t, int32_t>>{300};
template <> inline constexpr DType dtype<std::pair<int64_t, int64_t>>{301};
template <>
inline constexpr DType dtype<std::unordered_map<double, int64_t>>{302};
template <>
inline constexpr DType dtype<std::unordered_map<double, int32_t>>{303};
template <>
inline constexpr DType dtype<std::unordered_map<float, int64_t>>{304};
template <>
inline constexpr DType dtype<std::unordered_map<float, int32_t>>{305};
template <>
inline constexpr DType dtype<std::unordered_map<int64_t, int64_t>>{306};
template <>
inline constexpr DType dtype<std::unordered_map<int64_t, int32_t>>{307};
template <>
inline constexpr DType dtype<std::unordered_map<int32_t, int64_t>>{308};
template <>
inline constexpr DType dtype<std::unordered_map<int32_t, int32_t>>{309};
template <>
inline constexpr DType dtype<std::unordered_map<bool, int64_t>>{310};
template <>
inline constexpr DType dtype<std::unordered_map<bool, int32_t>>{311};
template <>
inline constexpr DType dtype<std::unordered_map<std::string, int64_t>>{312};
template <>
inline constexpr DType dtype<std::unordered_map<std::string, int32_t>>{313};
template <>
inline constexpr DType dtype<std::unordered_map<core::time_point, int64_t>>{
    314};
template <>
inline constexpr DType dtype<std::unordered_map<core::time_point, int32_t>>{
    315};
// scipp::variable types start at 1000
// scipp::dataset types start at 2000
// scipp::python types start at 3000
// Eigen types start at 4000
// Spatial transform types start at 5000
// User types should start at 10000

SCIPP_CORE_EXPORT bool is_int(DType tp);
SCIPP_CORE_EXPORT bool is_float(DType tp);
SCIPP_CORE_EXPORT bool is_fundamental(DType tp);
SCIPP_CORE_EXPORT bool is_total_orderable(DType tp);
SCIPP_CORE_EXPORT bool is_span(DType tp);
SCIPP_CORE_EXPORT bool is_structured(DType tp);

template <class T> constexpr bool canHaveVariances() noexcept {
  using U = std::remove_const_t<T>;
  return std::is_same_v<U, double> || std::is_same_v<U, float> ||
         std::is_same_v<U, scipp::span<const double>> ||
         std::is_same_v<U, scipp::span<const float>> ||
         std::is_same_v<U, scipp::span<double>> ||
         std::is_same_v<U, scipp::span<float>>;
}

SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const DType &dtype);

} // namespace scipp::core

namespace scipp {
using core::DType;
using core::dtype;
} // namespace scipp

namespace std {
template <> struct hash<scipp::DType> {
  std::size_t operator()(const scipp::DType &dt) const noexcept {
    return std::hash<decltype(dt.index)>{}(dt.index);
  }
};
} // namespace std
