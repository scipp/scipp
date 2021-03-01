// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <Eigen/Dense>
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
template <> constexpr DType dtype<void>{0};
template <> constexpr DType dtype<double>{1};
template <> constexpr DType dtype<float>{2};
template <> constexpr DType dtype<int64_t>{3};
template <> constexpr DType dtype<int32_t>{4};
template <> constexpr DType dtype<bool>{5};
template <> constexpr DType dtype<std::string>{6};
template <> constexpr DType dtype<time_point>{7};
template <> constexpr DType dtype<Eigen::Vector3d>{8};
template <> constexpr DType dtype<Eigen::Matrix3d>{9};
class SubbinSizes;
template <> constexpr DType dtype<SubbinSizes>{10};
// span<T> start at 100
template <> constexpr DType dtype<span<const double>>{100};
template <> constexpr DType dtype<span<const float>>{101};
template <> constexpr DType dtype<span<const int64_t>>{102};
template <> constexpr DType dtype<span<const int32_t>>{103};
template <> constexpr DType dtype<span<const bool>>{104};
template <> constexpr DType dtype<span<const std::string>>{105};
template <> constexpr DType dtype<span<const time_point>>{106};
template <> constexpr DType dtype<span<const Eigen::Vector3d>>{107};
// span<const T> start at 200
template <> constexpr DType dtype<span<double>>{200};
template <> constexpr DType dtype<span<float>>{201};
template <> constexpr DType dtype<span<int64_t>>{202};
template <> constexpr DType dtype<span<int32_t>>{203};
template <> constexpr DType dtype<span<bool>>{204};
template <> constexpr DType dtype<span<std::string>>{205};
template <> constexpr DType dtype<span<time_point>>{206};
template <> constexpr DType dtype<span<Eigen::Vector3d>>{207};
// std containers start at 300
template <> constexpr DType dtype<std::pair<int32_t, int32_t>>{300};
template <> constexpr DType dtype<std::pair<int64_t, int64_t>>{301};
template <> constexpr DType dtype<std::unordered_map<double, int64_t>>{302};
template <> constexpr DType dtype<std::unordered_map<double, int32_t>>{303};
template <> constexpr DType dtype<std::unordered_map<float, int64_t>>{304};
template <> constexpr DType dtype<std::unordered_map<float, int32_t>>{305};
template <> constexpr DType dtype<std::unordered_map<int64_t, int64_t>>{306};
template <> constexpr DType dtype<std::unordered_map<int64_t, int32_t>>{307};
template <> constexpr DType dtype<std::unordered_map<int32_t, int64_t>>{308};
template <> constexpr DType dtype<std::unordered_map<int32_t, int32_t>>{309};
template <> constexpr DType dtype<std::unordered_map<bool, int64_t>>{310};
template <> constexpr DType dtype<std::unordered_map<bool, int32_t>>{311};
template <>
constexpr DType dtype<std::unordered_map<std::string, int64_t>>{312};
template <>
constexpr DType dtype<std::unordered_map<std::string, int32_t>>{313};
template <>
constexpr DType dtype<std::unordered_map<core::time_point, int64_t>>{314};
template <>
constexpr DType dtype<std::unordered_map<core::time_point, int32_t>>{315};
// scipp::variable types start at 1000
// scipp::dataset types start at 2000
// scipp::python types start at 3000
// User types should start at 10000

SCIPP_CORE_EXPORT bool isInt(DType tp);
SCIPP_CORE_EXPORT bool is_span(DType tp);

template <class T> constexpr bool canHaveVariances() noexcept {
  using U = std::remove_const_t<T>;
  return std::is_same_v<U, double> || std::is_same_v<U, float> ||
         std::is_same_v<U, span<const double>> ||
         std::is_same_v<U, span<const float>> ||
         std::is_same_v<U, span<double>> || std::is_same_v<U, span<float>>;
}

SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const DType &dtype);

} // namespace scipp::core

namespace scipp {
using core::DType;
using core::dtype;
} // namespace scipp
