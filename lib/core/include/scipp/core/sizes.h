// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <span>

// Warnings are raised by boost small_vector with gcc12
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
#include <boost/container/small_vector.hpp>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/slice.h"
#include "scipp/units/dim.h"

namespace scipp::core {

/// Maximum number of dimensions supported by transform-based operations
constexpr int32_t NDIM_OP_MAX = 6;
/// Number of dimension labels/sizes/strides storable without heap allocation
constexpr int32_t NDIM_STACK = 4;

class Dimensions;

/// Small (fixed maximum size) and stable (preserving key order) map
template <class Key, class Value, int16_t Capacity>
class SCIPP_CORE_EXPORT small_stable_map {
public:
  static constexpr auto capacity = Capacity;

  small_stable_map() = default;

  bool operator==(const small_stable_map &other) const noexcept;
  bool operator!=(const small_stable_map &other) const noexcept;

  [[nodiscard]] auto begin() const noexcept { return m_keys.begin(); }
  [[nodiscard]] auto end() const noexcept { return m_keys.begin() + size(); }
  [[nodiscard]] auto rbegin() const noexcept {
    return std::reverse_iterator(end());
  }
  [[nodiscard]] auto rend() const noexcept {
    return std::reverse_iterator(begin());
  }
  [[nodiscard]]
  typename boost::container::small_vector<Key, Capacity>::const_iterator
  find(const Key &key) const;
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }
  [[nodiscard]] scipp::index size() const noexcept { return m_keys.size(); }
  [[nodiscard]] bool contains(const Key &key) const;
  [[nodiscard]] scipp::index index(const Key &key) const;
  [[nodiscard]] const Value &operator[](const Key &key) const;
  [[nodiscard]] const Value &at(const Key &key) const;
  void assign(const Key &key, const Value &value);
  void insert_left(const Key &key, const Value &value);
  void insert_right(const Key &key, const Value &value);
  void erase(const Key &key);
  void clear() noexcept;
  void replace_key(const Key &from, const Key &to);
  [[nodiscard]] std::span<const Key> keys() const & noexcept {
    return {m_keys.data(), static_cast<size_t>(size())};
  }
  [[nodiscard]] std::span<const Value> values() const & noexcept {
    return {m_values.data(), static_cast<size_t>(size())};
  }

private:
  boost::container::small_vector<Key, Capacity> m_keys{};
  boost::container::small_vector<Value, Capacity> m_values{};
};

/// Similar to class Dimensions but without implied ordering
class SCIPP_CORE_EXPORT Sizes
    : public small_stable_map<Dim, scipp::index, NDIM_STACK> {
private:
  using base = small_stable_map<Dim, scipp::index, NDIM_STACK>;

protected:
  using base::assign;
  using base::insert_left;
  using base::insert_right;

public:
  Sizes() noexcept = default;

  void set(const Dim dim, const scipp::index size);
  void resize(const Dim dim, const scipp::index size);
  [[nodiscard]] bool includes(const Sizes &sizes) const;
  [[nodiscard]] Sizes slice(const Slice &params) const;

  /// Return the labels of the space defined by *this.
  [[nodiscard]] auto labels() const & noexcept { return keys(); }
  /// Return the shape of the space defined by *this.
  [[nodiscard]] auto sizes() const & noexcept { return values(); }

  [[nodiscard]] Sizes rename_dims(const std::vector<std::pair<Dim, Dim>> &names,
                                  const bool fail_on_unknown = true) const;
};

[[nodiscard]] SCIPP_CORE_EXPORT Sizes concat(const std::span<const Sizes> sizes,
                                             const Dim dim);

[[nodiscard]] SCIPP_CORE_EXPORT Sizes merge(const Sizes &a, const Sizes &b);

SCIPP_CORE_EXPORT bool is_edges(const Sizes &sizes, const Sizes &dataSizes,
                                const Dim dim);

} // namespace scipp::core

namespace scipp {
using core::Sizes;
}
