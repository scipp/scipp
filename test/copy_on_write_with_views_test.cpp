/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <gsl/gsl_util>
#include <memory>
#include <mutex>
#include <omp.h>
#include <vector>

#include "../benchmark/legacy_cow_ptr.h"

template <class T> class BufferManager {
public:
  BufferManager(const gsl::index size)
      : m_data(std::make_shared<cow_ptr<T>>(std::make_unique<T>(size))) {}
  BufferManager(const BufferManager &other)
      : m_data(std::make_shared<cow_ptr<T>>(*other.m_data)) {}

  const T &data() const { return *m_data; }
  T &mutableData() { return m_data.access(); }

  const T &getForReading(std::shared_ptr<cow_ptr<T>> &keepAlive) const {
    // Two things need to be protected by mutex:
    // 1. The outer pointer may not be copied and reassigned at the same time.
    // 2. The inner pointer may not be copied and used with `access()` at the
    // same time.
    if (keepAlive != m_data) {
      std::lock_guard<std::mutex> lock{m_mutex};
      keepAlive = m_data;
    }
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
      std::lock_guard<std::mutex> lock{m_mutex};
      if (!m_data->unique()) {
        // Dropping old buffer. This is strictly speaking not necessary but can
        // avoid unneccessary copies of the buffer owner (not the buffer
        // itself).
        keepAlive = nullptr;
        if (!m_data.unique()) {
          std::atomic_store(&m_data, std::make_shared<cow_ptr<T>>(*m_data));
        }
      }
      keepAlive = m_data;
      // Do not move this outside the `if`, we need the lock here!
      return keepAlive->access();
    } else {
      if (keepAlive != m_data) {
        // TODO Do we need this lock? Why? Why not? Do we need to lock for the
        // comparison? Same for getForReading().
        std::lock_guard<std::mutex> lock{m_mutex};
        keepAlive = m_data;
      }
      return keepAlive->access();
    }
  }

private:
  std::shared_ptr<cow_ptr<T>> m_data;
  mutable std::mutex m_mutex;
};

template <class T> class VariableView {
public:
  VariableView(const gsl::index size)
      : m_bufferManager(std::make_unique<BufferManager<T>>(size)) {}

  // This *copies* the BufferManager.
  VariableView(const VariableView &other)
      : m_bufferManager(
            std::make_shared<BufferManager<T>>(*other.m_bufferManager)) {}

  // This creates a view, *sharing* the BufferManager.
  VariableView makeView() { return VariableView(m_bufferManager); }

  const T &data() const {
    // Updating the mutable m_bufferKeepAlive should be thread-safe due to mutex
    // in m_bufferManager?
    return m_bufferManager->getForReading(m_bufferKeepAlive);
  }

  T &mutableData() { return m_bufferManager->getForWriting(m_bufferKeepAlive); }

private:
  VariableView(std::shared_ptr<BufferManager<T>> bufferManager)
      : m_bufferManager(bufferManager) {}

  std::shared_ptr<BufferManager<T>> m_bufferManager;
  mutable std::shared_ptr<cow_ptr<T>> m_bufferKeepAlive;
};

TEST(VariableView, read_write) {
  VariableView<std::vector<double>> v(4);

  // Read
  ASSERT_EQ(v.data().size(), 4);
  EXPECT_EQ(v.data()[0], 0.0);

  // Write
  v.mutableData()[0] = 1.0;
  EXPECT_EQ(v.data()[0], 1.0);
}

TEST(VariableView, copy) {
  VariableView<std::vector<double>> v(4);
  v.mutableData()[0] = 1.0;

  // Copy does not see changes
  auto copy(v);
  EXPECT_EQ(copy.data()[0], 1.0);
  v.mutableData()[0] = 2.0;
  EXPECT_EQ(copy.data()[0], 1.0);
}

TEST(VariableView, view) {
  VariableView<std::vector<double>> v(4);
  v.mutableData()[0] = 1.0;

  // View sees changes.
  auto view = v.makeView();
  EXPECT_EQ(view.data()[0], 1.0);
  v.mutableData()[0] = 2.0;
  EXPECT_EQ(view.data()[0], 2.0);
}

TEST(VariableView, copy_and_view_write) {
  VariableView<std::vector<double>> v(4);
  v.mutableData()[0] = 1.0;

  auto copy(v);
  auto view = v.makeView();

  v.mutableData()[0] = 2.0;

  EXPECT_EQ(copy.data()[0], 1.0);
  EXPECT_EQ(view.data()[0], 2.0);
}

TEST(VariableView, copy_and_view_write_to_copy) {
  VariableView<std::vector<double>> v(4);
  v.mutableData()[0] = 1.0;

  auto copy(v);
  auto view = v.makeView();

  copy.mutableData()[0] = 2.0;

  EXPECT_EQ(view.data()[0], 1.0);
  EXPECT_EQ(v.data()[0], 1.0);
}

TEST(VariableView, copy_and_view_write_to_view) {
  VariableView<std::vector<double>> v(4);
  v.mutableData()[0] = 1.0;

  auto copy(v);
  auto view = v.makeView();

  view.mutableData()[0] = 2.0;

  EXPECT_EQ(copy.data()[0], 1.0);
  EXPECT_EQ(v.data()[0], 2.0);
}

TEST(VariableView, single_owner_read_then_write) {
  VariableView<std::vector<double>> v(4);
  v.mutableData()[0] = 1.0;

  const auto *addr = v.data().data();
  EXPECT_EQ(v.data()[0], 1.0);
  v.mutableData()[0] = 2.0;

  EXPECT_EQ(v.data()[0], 2.0);
  EXPECT_EQ(v.data().data(), addr);
}

TEST(VariableView, multiple_owners_read_then_write) {
  VariableView<std::vector<double>> v(4);
  v.mutableData()[0] = 1.0;

  auto copy(v);

  EXPECT_EQ(v.data()[0], 1.0);
  v.mutableData()[0] = 2.0;

  EXPECT_EQ(copy.data()[0], 1.0);
  EXPECT_EQ(v.data()[0], 2.0);
}

TEST(VariableView, thread_safety_multiple_writers_multiple_view) {
  const gsl::index chunks = 12345;
  const gsl::index chunk_size = 321;
  const gsl::index size = chunk_size * chunks;
  for (int repeat = 0; repeat < 512; ++repeat) {
    VariableView<std::vector<double>> v(size);
    auto copy(v);
#pragma omp parallel for num_threads(49)
    for (gsl::index chunk = 0; chunk < chunks; ++chunk) {
      auto view = v.makeView();
      view.data();
      auto &data = view.mutableData();
      for (gsl::index i = 0; i < chunk_size; ++i) {
        data[chunk * chunk_size + i] = chunk * chunk_size + i;
      }
    }

    const auto &data = v.data();
    for (gsl::index i = 0; i < size; ++i)
      ASSERT_EQ(data[i], i);
    const auto &copy_data = copy.data();
    for (gsl::index i = 0; i < size; ++i)
      ASSERT_EQ(copy_data[i], 0.0);
  }
}

TEST(VariableView, thread_safety_multi_writers_same_view) {
  const gsl::index chunks = 12345;
  const gsl::index chunk_size = 321;
  const gsl::index size = chunk_size * chunks;
  for (int repeat = 0; repeat < 512; ++repeat) {
    VariableView<std::vector<double>> v(size);
    auto copy(v);
    auto view = v.makeView();
#pragma omp parallel for num_threads(49)
    for (gsl::index chunk = 0; chunk < chunks; ++chunk) {
      view.data();
      auto &data = view.mutableData();
      for (gsl::index i = 0; i < chunk_size; ++i) {
        data[chunk * chunk_size + i] = chunk * chunk_size + i;
      }
    }

    const auto &data = v.data();
    for (gsl::index i = 0; i < size; ++i)
      ASSERT_EQ(data[i], i);
    const auto &copy_data = copy.data();
    for (gsl::index i = 0; i < size; ++i)
      ASSERT_EQ(copy_data[i], 0.0);
  }
}
