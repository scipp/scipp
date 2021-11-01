// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

[[noreturn]] inline void throw_nesting_error(const DataArray &) {
  throw std::invalid_argument("Cannot assign DataArray, the right hand side "
                              "contains a reference to the left hand side. "
                              "Reference cycles are not allowed.");
}

[[noreturn]] inline void throw_nesting_error(const Dataset &) {
  throw std::invalid_argument("Cannot assign Dataset, the right hand side "
                              "contains a reference to the left hand side. "
                              "Reference cycles are not allowed.");
}

template <class L, class R>
void check_nested_in_assign(const L &lhs, const R &rhs);

template <class T>
void check_nested_in_assign(const T &lhs, const Variable &rhs) {
  if (rhs.dtype() == dtype<DataArray>) {
    for (const auto &nested : rhs.values<DataArray>()) {
      check_nested_in_assign(lhs, nested);
    }
  } else if (rhs.dtype() == dtype<Dataset>) {
    for (const auto &nested : rhs.values<Dataset>()) {
      check_nested_in_assign(lhs, nested);
    }
  }
}

template <class T, class Key, class Value>
void check_nested_in_assign(const T &lhs, const Dict<Key, Value> &rhs) {
  for (const auto &[_, var] : rhs) {
    check_nested_in_assign(lhs, var);
  }
}

template <class L, class R>
void check_nested_in_assign(const L &lhs, const R &rhs) {
  if constexpr (std::is_same_v<L, R>) {
    if (&lhs == &rhs) {
      throw_nesting_error(lhs);
    }
  }

  const auto check_item = [&](const DataArray &item) {
    if (!item.is_valid()) {
      return;
    }
    check_nested_in_assign(lhs, item.data());
    check_nested_in_assign(lhs, item.coords());
    check_nested_in_assign(lhs, item.masks());
    check_nested_in_assign(lhs, item.attrs());
  };

  if constexpr (std::is_same_v<std::decay_t<R>, DataArray>) {
    check_item(rhs);
  } else {
    for (const auto &item : rhs) {
      check_item(item);
    }
    check_nested_in_assign(lhs, rhs.coords());
  }
}

} // namespace scipp::dataset
