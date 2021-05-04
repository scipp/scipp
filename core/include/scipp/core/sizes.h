// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <unordered_map>

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/slice.h"
#include "scipp/units/dim.h"

namespace scipp::core {

template <class Key, class Value, int16_t MaxSize>
class SCIPP_CORE_EXPORT small_map {
public:
  small_map() = default;

  bool operator==(const small_map &other) const noexcept;
  bool operator!=(const small_map &other) const noexcept;

  auto begin() const { return m_keys.begin(); }
  auto end() const { return m_keys.begin() + size(); }
  typename std::array<Key, MaxSize>::const_iterator find(const Key &key) const;
  [[nodiscard]] bool empty() const noexcept;
  scipp::index size() const noexcept { return m_size; }
  bool contains(const Key &key) const noexcept;
  const Value &operator[](const Key &key) const;
  Value &operator[](const Key &key);
  const Value &at(const Key &key) const;
  Value &at(const Key &key);
  void insert(const Key &key, const Value &value);
  void erase(const Key &key);
  void clear();
  // void relabel(const Dim from, const Dim to);

private:
  int64_t m_size{0};
  std::array<Key, MaxSize> m_keys{};
  std::array<Value, MaxSize> m_values{};
};

/// Sibling of class Dimensions, but unordered.
class SCIPP_CORE_EXPORT Sizes : public small_map<Dim, scipp::index, NDIM_MAX> {
private:
  using base = small_map<Dim, scipp::index, NDIM_MAX>;

public:
  Sizes() = default;
  Sizes(const Dimensions &dims);
  Sizes(const std::unordered_map<Dim, scipp::index> &sizes);

  scipp::index count(const Dim dim) const noexcept { return contains(dim); }

  void set(const Dim dim, const scipp::index size);
  void relabel(const Dim from, const Dim to);
  using base::contains;
  bool contains(const Dimensions &dims) const;
  bool contains(const Sizes &sizes) const;
  Sizes slice(const Slice &params) const;
};

[[nodiscard]] SCIPP_CORE_EXPORT Sizes concatenate(const Sizes &a,
                                                  const Sizes &b,
                                                  const Dim dim);

[[nodiscard]] SCIPP_CORE_EXPORT Sizes merge(const Sizes &a, const Sizes &b);

SCIPP_CORE_EXPORT bool is_edges(const Sizes &sizes, const Dimensions &dims,
                                const Dim dim);

SCIPP_CORE_EXPORT std::string to_string(const Sizes &sizes);

} // namespace scipp::core

namespace scipp {
using core::Sizes;
}
