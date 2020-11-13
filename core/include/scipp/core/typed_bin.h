// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <algorithm>
#include <optional>

#include "scipp/common/span.h"

namespace scipp::core {

template <class T> class typed_bin {
public:
  using view_type = typed_bin;
  using const_view_type = typed_bin;

  typed_bin(const scipp::span<T> values) : m_values(values) {}
  typed_bin(const scipp::span<T> values, const scipp::span<T> variances)
      : m_values(values), m_variances(variances) {}

  constexpr auto size() const noexcept { return scipp::size(m_values); }
  auto values() const noexcept { return m_values; }
  auto variances() const noexcept { return m_variances; };

  /// Return a subspan
  typed_bin operator()(const scipp::index_pair &range) const {
    if (m_variances)
      return {m_values.subspan(range.first, range.second - range.first),
              m_variances->subspan(range.first, range.second - range.first)};
    else
      return {m_values.subspan(range.first, range.second - range.first)};
  }

private:
  scipp::span<T> m_values;
  std::optional<scipp::span<T>> m_variances;
};

template <class T>
constexpr bool operator==(const typed_bin<T> &a,
                          const typed_bin<T> &b) noexcept {
  constexpr auto equal = [](const auto &a_, const auto &b_) {
    return std::equal(a_.begin(), a_.end(), b_.begin(), b_.end());
  };
  return a.variances().has_value() == b.variances().has_value() &&
         equal(a.values(), b.values()) &&
         (a.variances() ? equal(*a.variances(), *b.variances()) : true);
}

} // namespace scipp::core

namespace scipp {
using core::typed_bin;
template <class T> struct is_typed_bin : std::false_type {};
template <class T> struct is_typed_bin<typed_bin<T>> : std::true_type {};
template <class T>
inline constexpr bool is_typed_bin_v = is_typed_bin<T>::value;
} // namespace scipp
