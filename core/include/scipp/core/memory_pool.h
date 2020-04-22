// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <array>
#include <map>
#include <mutex>
#include <vector>

#include "scipp/common/index.h"

namespace scipp::core {
#ifdef _WIN32
// https://stackoverflow.com/questions/33696092/whats-the-correct-replacement-for-posix-memalign-in-windows
static int check_align(size_t align) {
  for (size_t i = sizeof(void *); i != 0; i *= 2)
    if (align == i)
      return 0;
  return EINVAL;
}

static int posix_memalign(void **ptr, size_t align, size_t size) {
  if (check_align(align))
    return EINVAL;

  int saved_errno = errno;
  void *p = _aligned_malloc(size, align);
  if (p == NULL) {
    errno = saved_errno;
    return ENOMEM;
  }

  *ptr = p;
  return 0;
}
#endif

class MemoryPool {
public:
  void *allocate(size_t size) const {
    std::lock_guard<std::mutex> g(m_mutex);
    auto &pools = m_pools[size];
    if (pools[1].empty()) {
      void *ptr = nullptr;
      // Always 64 bit aligned
      int rc = posix_memalign(&ptr, 64, size);
      if (rc != 0)
        throw std::runtime_error("Failed to allocate.");
      pools[0].emplace_back(ptr);
      return ptr;
    } else {
      pools[0].emplace_back(pools[1].back());
      pools[1].pop_back();
      return pools[0].back();
    }
  }
  void deallocate(void *ptr) const {
    std::lock_guard<std::mutex> g(m_mutex);
    for (auto &pools : m_pools) {
      auto &used = pools.second[0];
      for (scipp::index i = used.size() - 1; i >= 0; --i) {
        if (used[i] == ptr) {
          auto &unused = pools.second[1];
          used.erase(used.begin() + i);
          unused.emplace_back(ptr);
          return;
        }
      }
    }
    throw std::runtime_error(
        "Request to deallocate memory that is not owned by us.");
  }
  ~MemoryPool() {
    for (auto &pools : m_pools) {
      for (auto &pool : pools.second)
        for (auto &ptr : pool)
#ifdef _WIN32
          _aligned_free(ptr);
#else
          free(ptr);
#endif
    }
  }

private:
  mutable std::mutex m_mutex;
  // Different pool for each size.
  mutable std::map<size_t, std::array<std::vector<void *>, 2>> m_pools;
};

inline MemoryPool &instance() {
  static MemoryPool pool;
  return pool;
}

} // namespace scipp::core
