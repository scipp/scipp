// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <boost/container/small_vector.hpp>
#include <iosfwd>
#include <typeindex>

#include "scipp-core_export.h"
#include "scipp/common/span.h"
#include "scipp/core/time_point.h"

namespace scipp::core {

struct SCIPP_CORE_EXPORT DType {
  std::type_index index;
  bool operator==(const DType &t) const noexcept { return index == t.index; }
  bool operator!=(const DType &t) const noexcept { return index != t.index; }
  bool operator<(const DType &t) const noexcept { return index < t.index; }
};
template <class T> inline DType dtype{std::type_index(typeid(T))};

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
