// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/except.h"

namespace scipp::core {

/// A values/variances pair based on references to event listss.
///
/// This is a helper for implementing operations for event_list such as
/// `clear`, and for descending into the event list itself, using a nested
/// call to an iteration function.
template <class T> struct ValuesAndVariances {
  using value_type = typename T::value_type;

  ValuesAndVariances(T &val, T &var) : values(val), variances(var) {
    expect::sizeMatches(values, variances);
  }
  ValuesAndVariances &operator=(std::pair<T, T> &&data) {
    values = std::move(data.first);
    variances = std::move(data.second);
    return *this;
  }
  T &values;
  T &variances;

  void clear() noexcept {
    values.clear();
    variances.clear();
  }

  void reserve(const scipp::index capacity) const {
    values.reserve(capacity);
    variances.reserve(capacity);
  }

  // Note that methods like insert, begin, and end are required as long as we
  // support event data via a plain container such as std::vector, e.g., for
  // concatenation using a.insert(a.end(), b.begin(), b.end()). We are
  // supporting this here by simply working with pairs of iterators. This
  // approach is not an actual view iterator and will not compile if client
  // code attempts to increment the iterators. We could support `next` and
  // `advance` easily, so client code can simply use something like:
  //   using std::next;
  //   next(it);
  // instead of `++it`. Algorithms like `std::sort` would probably still not
  // work though.
  // The function arguments are iterator pairs as created by `begin` and `end`.
  template <class OutputIt, class InputIt>
  auto insert(std::pair<OutputIt, OutputIt> pos,
              std::pair<InputIt, InputIt> first,
              std::pair<InputIt, InputIt> last) {
    values.insert(pos.first, first.first, last.first);
    variances.insert(pos.second, first.second, last.second);
  }
  template <class... Ts> void insert(const Ts &...) {
    throw except::VariancesError("Cannot insert data with variances into data "
                                 "without variances, or vice versa.");
  }

  auto begin() noexcept { return std::pair(values.begin(), variances.begin()); }
  auto begin() const noexcept {
    return std::pair(values.begin(), variances.begin());
  }
  auto end() noexcept { return std::pair(values.end(), variances.end()); }
  auto end() const noexcept { return std::pair(values.end(), variances.end()); }

  constexpr auto size() const noexcept { return values.size(); }
};

template <class T> struct is_ValuesAndVariances : std::false_type {};
template <class T>
struct is_ValuesAndVariances<ValuesAndVariances<T>> : std::true_type {};
template <class T>
inline constexpr bool is_ValuesAndVariances_v = is_ValuesAndVariances<T>::value;

} // namespace scipp::core

namespace scipp {
using core::is_ValuesAndVariances;
using core::is_ValuesAndVariances_v;
using core::ValuesAndVariances;
} // namespace scipp
