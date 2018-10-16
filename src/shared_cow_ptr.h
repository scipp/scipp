#include <gsl/gsl_util>
#include <memory>
#include <mutex>

#include "cow_ptr.h"

template <class T> class BufferManager {
public:
  BufferManager(std::unique_ptr<T> object)
      : m_data(std::make_shared<cow_ptr<T>>(std::move(object))) {}
  BufferManager(const BufferManager &other)
      : m_data(std::make_shared<cow_ptr<T>>(*other.m_data)) {}

  const T &data() const { return *m_data; }
  T &mutableData() { return m_data.access(); }

  const T &getForReading(std::shared_ptr<cow_ptr<T>> &keepAlive) const {
    // Two things need to be protected by mutex:
    // 1. The outer pointer may not be copied and reassigned at the same time.
    // 2. The inner pointer may not be copied and used with `access()` at the
    // same time.
    if (keepAlive != m_data)
      std::atomic_store(&keepAlive, m_data);
    return **keepAlive;
  }

  T &getForWriting(std::shared_ptr<cow_ptr<T>> &keepAlive) {
    // If buffer owner (m_data) is unique this will simply behave like a normal
    // copy-on-write (the inner call to access()). If there are multiple buffer
    // owners it first copies the owner (the outer access()) to ensure that
    // other owners can still read in a safe way.
    // Note the difference to using cow_ptr<cow_ptr>: Here we copy the outer
    // based on the ref count of the *inner* pointer!
    if (!m_data->unique()) {
      // TODO Can use atomics instead of lock?
      std::lock_guard<std::mutex> lock{m_mutex};
      if (!m_data->unique()) {
        // Dropping old buffer. This is strictly speaking not necessary but can
        // avoid unneccessary copies of the buffer owner (not the buffer
        // itself).
        std::atomic_store(&keepAlive, std::shared_ptr<cow_ptr<T>>(nullptr));
        if (!m_data.unique()) {
          std::atomic_store(&m_data, std::make_shared<cow_ptr<T>>(*m_data));
        }
      }
      std::atomic_store(&keepAlive, m_data);
      // Do not move this outside the `if`, we need the lock here!
      return keepAlive->access();
    } else {
      if (keepAlive != m_data)
        std::atomic_store(&keepAlive, m_data);
      return keepAlive->access();
    }
  }

  bool operator==(const BufferManager<T> &other) const noexcept {
    // Note: Equality of data pointer is checked here.
    return *m_data == *other.m_data;
  }
  bool unique() const noexcept { return (*m_data).unique(); }

private:
  std::shared_ptr<cow_ptr<T>> m_data;
  mutable std::mutex m_mutex;
};

// "Sharable" by multiple views on the same data.
template <class T> class shared_cow_ptr {
public:
  shared_cow_ptr(std::unique_ptr<T> object)
      : m_bufferManager(std::make_shared<BufferManager<T>>(std::move(object))) {
  }
  shared_cow_ptr(const shared_cow_ptr &other)
      : m_bufferManager(
            std::make_shared<BufferManager<T>>(*other.m_bufferManager)) {}
  shared_cow_ptr(shared_cow_ptr &&) = default;
  shared_cow_ptr &operator=(const shared_cow_ptr &other) {
    m_bufferManager =
        std::make_shared<BufferManager<T>>(*other.m_bufferManager);
  }
  shared_cow_ptr &operator=(shared_cow_ptr &&) = default;

  const T &operator*() const {
    return m_bufferManager->getForReading(m_bufferKeepAlive);
  }
  const T *operator->() const {
    return &m_bufferManager->getForReading(m_bufferKeepAlive);
  }
  T &access() { return m_bufferManager->getForWriting(m_bufferKeepAlive); }
  bool operator==(const shared_cow_ptr<T> &other) const noexcept {
    return *m_bufferManager == *other.m_bufferManager;
  }
  bool unique() const noexcept { return m_bufferManager->unique(); }

private:
  std::shared_ptr<BufferManager<T>> m_bufferManager;
  mutable std::shared_ptr<cow_ptr<T>> m_bufferKeepAlive;
};
