/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <gsl/gsl_util>
#include <memory>
#include <vector>

#include "../benchmark/legacy_cow_ptr.h"

template <class T> class BufferManager {
public:
  BufferManager(const gsl::index size) : m_data(std::make_unique<T>(size)) {}

  const T &data() const { return *m_data; }
  T &mutableData() { return m_data.access(); }

  std::shared_ptr<cow_ptr<T>> getForReading() const { return m_data; }
  std::shared_ptr<cow_ptr<T>> getForWriting() {
    if(m_data.unique()) {
      // Call *inner* access to copy buffer.
      m_data->access();
    } else {
      if (m_data->unique()) {
        // If buffer is unique, do nothing.
      } else {
        // More than one view, may not call access() for current buffer so we
        // first copy the buffer owner.
        m_data.access().access();
      }
    }
    return m_data;
  }

private:
  std::shared_ptr<cow_ptr<T>> m_data;
};

template <class T> class VariableView {
public:
  VariableView(const gsl::index size)
      : m_bufferManager(std::make_unique<BufferManager<T>>(size)) {
  }

  VariableView(const VariableView &other)
      : m_bufferManager(std::make_shared<BufferManager<T>>(
            *other.m_bufferManager)) {}

  VariableView makeView() { return VariableView(m_bufferManager); }

  const T &data() const {
    m_bufferKeepAlive = m_bufferManager->getForReading();
    return **m_bufferKeepAlive;
  }
  T &mutableData() { 
    m_bufferKeepAlive = m_bufferManager->getForWriting();
    return *m_bufferManager; }

private:
  VariableView(std::shared_ptr<BufferManager<T>> bufferManager)
      : m_bufferManager(bufferManager) {}

  std::shared_ptr<BufferManager<T>> m_bufferManager;
  std::shared_ptr<cow_ptr<T>> m_bufferKeepAlive;
};

TEST(VariableView, copy_and_view) {
  VariableView<std::vector<double>> v(4);

  // Read
  ASSERT_EQ(v.data().size(), 4);
  EXPECT_EQ(v.data()[0], 0.0);

  // Write
  v.mutableData()[0] = 1.0;
  EXPECT_EQ(v.data()[0], 1.0);

  // Copy does not see changes
  auto copy(v);
  v.mutableData()[0] = 2.0;
  EXPECT_EQ(copy.data()[0], 1.0);

  // View sees changes.
  auto view = v.makeView();
  EXPECT_EQ(view.data()[0], 2.0);
  v.mutableData()[0] = 3.0;
  EXPECT_EQ(view.data()[0], 3.0);
}

TEST(VariableView, thread_safety) {
  VariableView<std::vector<double>> v1(4);
  v1.mutableData() = {1.0, 2.0, 3.0, 4.0};

  auto copy(v1);

  auto v2 = v1.makeView();
  auto v3 = v1.makeView();

  //
}
