#ifndef VECTOR_POOL_H
#define VECTOR_POOL_H

#include <gsl/gsl_util>

#include "vector.h"

template <class T> class VectorPool {
public:
  Vector<T> get(const gsl::index size) {
    std::lock_guard<std::mutex> g(m_mutex);
    auto it = std::find_if(
        m_pool.begin(), m_pool.end(),
        [size](const Vector<T> &vec) { return vec.size() == size; });
    if (it != m_pool.end()) {
      Vector<T> vec(std::move(*it));
    //fprintf(stderr, "Reusing vector of size %lu from pool. Pool size is now %lu\n", vec.size(), m_pool.size()-1);
      m_pool.erase(it);
      return vec;
    }
    return Vector<T>(size);
  }

  void put(Vector<T> &&vec) {
    std::lock_guard<std::mutex> g(m_mutex);
    if(m_pool.size() > 16)
      m_pool.clear();
    //fprintf(stderr, "Added vector of size %lu to pool. Pool size is now %lu\n", vec.size(), m_pool.size()+1);
    m_pool.emplace_back(std::move(vec));
  }

private:
  mutable std::mutex m_mutex;
  mutable std::vector<Vector<T>> m_pool;
};

template <class T> inline VectorPool<T> &vectorPoolInstance() {
  static VectorPool<T> pool;
  return pool;
}

#endif // VECTOR_POOL_H
