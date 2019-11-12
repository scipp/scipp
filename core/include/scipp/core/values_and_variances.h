// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#ifndef SCIPP_CORE_VALUES_AND_VARIANCES_H
#define SCIPP_CORE_VALUES_AND_VARIANCES_H

namespace scipp::core {

namespace detail {

/// A values/variances pair based on references to sparse data containers.
///
/// This is a helper for implementing operations for sparse container such as
/// `clear`, and for descending into the sparse container itself, using a nested
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

  void clear() {
    values.clear();
    variances.clear();
  }

  void reserve(const scipp::index capacity) const {
    values.reserve(capacity);
    variances.reserve(capacity);
  }

  // Note that methods like insert, begin, and end are required as long as we
  // support sparse data via a plain container such as std::vector, e.g., for
  // concatenation using a.insert(a.end(), b.begin(), b.end()). We are
  // supporting this here by simply working with pairs of iterators. This
  // approach is not an actual proxy iterator and will not compile if client
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
    throw std::runtime_error("Cannot insert data with variances into data "
                             "without variances, or vice versa.");
  }

  auto begin() { return std::pair(values.begin(), variances.begin()); }
  auto begin() const { return std::pair(values.begin(), variances.begin()); }
  auto end() { return std::pair(values.end(), variances.end()); }
  auto end() const { return std::pair(values.end(), variances.end()); }

  constexpr auto size() const noexcept { return values.size(); }
};

template <class T> struct is_ValuesAndVariances : std::false_type {};
template <class T>
struct is_ValuesAndVariances<ValuesAndVariances<T>> : std::true_type {};
template <class T>
inline constexpr bool is_ValuesAndVariances_v = is_ValuesAndVariances<T>::value;

} // namespace detail

} // namespace scipp::core

#endif // SCIPP_CORE_VALUES_AND_VARIANCES_H
