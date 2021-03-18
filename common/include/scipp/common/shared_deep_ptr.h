// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <memory>
#include <type_traits>

namespace scipp {

/// Like std::shared_ptr, but copy causes a deep copy.
template <class T> class shared_deep_ptr {
public:
  using element_type = typename std::unique_ptr<T>::element_type;
  shared_deep_ptr() = default;
  shared_deep_ptr(std::shared_ptr<T> other) : m_data(std::move(other)) {}
  shared_deep_ptr(const shared_deep_ptr<T> &other) {
    if (other) {
      if constexpr (std::is_abstract<T>())
        *this = other->clone();
      else
        m_data = std::make_shared<T>(*other);
    } else {
      m_data = nullptr;
    }
  }
  shared_deep_ptr(shared_deep_ptr<T> &&) = default;
  constexpr shared_deep_ptr(std::nullptr_t){};
  shared_deep_ptr<T> &operator=(const shared_deep_ptr<T> &other) {
    if (&other != this)
      *this = shared_deep_ptr(other);
    return *this;
  }
  shared_deep_ptr<T> &operator=(shared_deep_ptr<T> &&) = default;

  explicit operator bool() const noexcept { return bool(m_data); }
  bool operator==(const shared_deep_ptr<T> &other) const noexcept {
    return m_data == other.m_data;
  }
  bool operator!=(const shared_deep_ptr<T> &other) const noexcept {
    return m_data != other.m_data;
  }

  T &operator*() const { return *m_data; }
  T *operator->() const { return m_data.get(); }

  const std::shared_ptr<T> &owner() const { return m_data; }

private:
  std::shared_ptr<T> m_data;
};

} // namespace scipp
