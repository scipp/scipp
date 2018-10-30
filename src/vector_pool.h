#ifndef VECTOR_POOL_H
#define VECTOR_POOL_H

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
      Vector<T> vec;
      std::swap(vec, *it);
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
    auto it =
        std::find_if(m_pool.begin(), m_pool.end(),
                     [](const Vector<T> &vec) { return vec.size() == 0; });
    if (it != m_pool.end()) {
      std::swap(vec, *it);
    } else {
      std::swap(vec, m_pool[m_lastEvicted]);
      if (m_backgroundDealloc.joinable())
        m_backgroundDealloc.join();
      m_backgroundDealloc = std::thread([v = std::move(vec)]() mutable {});
      m_lastEvicted = (m_lastEvicted - 1 + m_pool.size()) % m_pool.size();
    }
  }

private:
  mutable std::mutex m_mutex;
  mutable std::vector<Vector<T>> m_pool{std::vector<Vector<T>>(16)};
  gsl::index m_lastEvicted{15};
  std::thread m_backgroundDealloc;
};

template <class T> inline VectorPool<T> &vectorPoolInstance() {
  static VectorPool<T> pool;
  return pool;
}

#endif // VECTOR_POOL_H
