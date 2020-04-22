// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

// from https://stackoverflow.com/a/12942652/1458281

#pragma once

#include <cassert>

#include "scipp/core/memory_pool.h"

namespace scipp::core {

enum class Alignment : size_t {
  Normal = sizeof(void *),
  SSE = 16,
  AVX = 32,
};

namespace detail {
void *allocate_aligned_memory(size_t align, size_t size);
void deallocate_aligned_memory(void *ptr) noexcept;

//#define USE_POOL

template <typename T> constexpr bool is_power_of_two(T v) {
  return v && ((v & (v - 1)) == 0);
}

inline void *allocate_aligned_memory(size_t align, size_t size) {
  assert(align >= sizeof(void *));
  assert(is_power_of_two(align));

  if (size == 0) {
    return nullptr;
  }

#ifdef USE_POOL
  return instance().allocate(size);
#else
  void *ptr = nullptr;
  int rc = posix_memalign(&ptr, align, size);
  if (rc != 0) {
    return nullptr;
  }
  return ptr;
#endif
}

inline void deallocate_aligned_memory(void *ptr) noexcept {
#ifdef USE_POOL
  return instance().deallocate(ptr);
#else
#ifdef _WIN32
  return _aligned_free(ptr);
#else
  return free(ptr);
#endif
#endif
}
} // namespace detail

template <typename T, Alignment Align = Alignment::AVX> class AlignedAllocator;

template <Alignment Align> class AlignedAllocator<void, Align> {
public:
  typedef void *pointer;
  typedef const void *const_pointer;
  typedef void value_type;

  template <class U> struct rebind {
    typedef AlignedAllocator<U, Align> other;
  };
};

template <typename T, Alignment Align> class AlignedAllocator {
public:
  typedef T value_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U> struct rebind {
    typedef AlignedAllocator<U, Align> other;
  };

public:
  AlignedAllocator() noexcept {}

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, Align> &) noexcept {}

  size_type max_size() const noexcept {
    return (size_type(~0) - size_type(Align)) / sizeof(T);
  }

  pointer address(reference x) const noexcept { return std::addressof(x); }

  const_pointer address(const_reference x) const noexcept {
    return std::addressof(x);
  }

  pointer allocate(size_type n,
                   typename AlignedAllocator<void, Align>::const_pointer = 0) {
    const size_type alignment = static_cast<size_type>(Align);
    void *ptr = detail::allocate_aligned_memory(alignment, n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    return detail::deallocate_aligned_memory(p);
  }

  template <class U, class... Args> void construct(U *p, Args &&... args) {
    ::new (reinterpret_cast<void *>(p)) U(std::forward<Args>(args)...);
  }

  void destroy(pointer p) { p->~T(); }
};

template <typename T, Alignment Align> class AlignedAllocator<const T, Align> {
public:
  typedef T value_type;
  typedef const T *pointer;
  typedef const T *const_pointer;
  typedef const T &reference;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U> struct rebind {
    typedef AlignedAllocator<U, Align> other;
  };

public:
  AlignedAllocator() noexcept {}

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, Align> &) noexcept {}

  size_type max_size() const noexcept {
    return (size_type(~0) - size_type(Align)) / sizeof(T);
  }

  const_pointer address(const_reference x) const noexcept {
    return std::addressof(x);
  }

  pointer allocate(size_type n,
                   typename AlignedAllocator<void, Align>::const_pointer = 0) {
    const size_type alignment = static_cast<size_type>(Align);
    void *ptr = detail::allocate_aligned_memory(alignment, n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    return detail::deallocate_aligned_memory(p);
  }

  template <class U, class... Args> void construct(U *p, Args &&... args) {
    ::new (reinterpret_cast<void *>(p)) U(std::forward<Args>(args)...);
  }

  void destroy(pointer p) { p->~T(); }
};

template <typename T, Alignment TAlign, typename U, Alignment UAlign>
inline bool operator==(const AlignedAllocator<T, TAlign> &,
                       const AlignedAllocator<U, UAlign> &) noexcept {
  return TAlign == UAlign;
}

template <typename T, Alignment TAlign, typename U, Alignment UAlign>
inline bool operator!=(const AlignedAllocator<T, TAlign> &,
                       const AlignedAllocator<U, UAlign> &) noexcept {
  return TAlign != UAlign;
}

} // namespace scipp::core
