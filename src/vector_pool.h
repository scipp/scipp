#ifndef VECTOR_POOL_H
#define VECTOR_POOL_H

#include <deque>
#include <thread>

#include <gsl/gsl_util>

#include "vector.h"

template <class T> class VectorPool {
public:
  ~VectorPool() {
    if (m_backgroundDealloc.joinable())
      m_backgroundDealloc.join();
  }

  Vector<T> get(const gsl::index size) {
    std::lock_guard<std::mutex> g(m_mutex);
    auto it = std::find_if(
        m_pool.begin(), m_pool.end(),
        [size](const Vector<T> &vec) { return vec.size() == size; });
    if (it != m_pool.end()) {
      Vector<T> vec(std::move(*it));
      m_pool.erase(it);
      // fprintf(stderr, "Reusing vector of size %lu from pool. Pool size is now
      // %lu\n", vec.size(), m_pool.size()-1);
      return vec;
    }
    return Vector<T>(size);
  }

  void put(Vector<T> &&vec) {
    std::lock_guard<std::mutex> g(m_mutex);
    // fprintf(stderr, "Added vector of size %lu to pool. Pool size is now
    // %lu\n", vec.size(), m_pool.size()+1);
    if (m_pool.size() == 8) {
      if (m_backgroundDealloc.joinable())
        m_backgroundDealloc.join();
      m_backgroundDealloc =
          std::thread([v = std::move(m_pool.back())]() mutable {});
      m_pool.pop_back();
    }
    m_pool.push_front(std::move(vec));
  }

private:
  std::mutex m_mutex;
  std::deque<Vector<T>> m_pool;
  std::thread m_backgroundDealloc;
};

template <class T> inline VectorPool<T> &vectorPoolInstance() {
  static VectorPool<T> pool;
  return pool;
}

#endif // VECTOR_POOL_H
