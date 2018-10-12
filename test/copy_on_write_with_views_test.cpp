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

  cow_ptr<cow_ptr<T>> getForReading() const { return m_data; }
  cow_ptr<cow_ptr<T>> getForWriting() {
    // If buffer owner (m_data) is unique this will simply behave like a normal
    // copy-on-write (the inner call to access()). If there are multiple buffer
    // owners it first copies the owner (the outer access()) to ensure that
    // other owners can still read in a safe way.
    // TODO Need lock here?
    if (!m_data->unique())
      m_data.access().access();
    return m_data;
  }

private:
  cow_ptr<cow_ptr<T>> m_data;
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
  cow_ptr<cow_ptr<T>> m_bufferKeepAlive;
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
