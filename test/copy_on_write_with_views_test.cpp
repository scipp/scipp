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

private:
  cow_ptr<T> m_data;
};

template <class T> class VariableView {
public:
  VariableView(const gsl::index size)
      : m_bufferManager(std::make_unique<BufferManager<std::vector<T>>>(size)) {
  }

  VariableView(const VariableView &other)
      : m_bufferManager(std::make_shared<BufferManager<std::vector<T>>>(
            *other.m_bufferManager)) {}

  VariableView makeView() { return VariableView(m_bufferManager); }

  const std::vector<T> &data() const { return m_bufferManager->data(); }
  std::vector<T> &mutableData() { return m_bufferManager->mutableData(); }

private:
  VariableView(std::shared_ptr<BufferManager<std::vector<T>>> bufferManager)
      : m_bufferManager(bufferManager) {}

  std::shared_ptr<BufferManager<std::vector<T>>> m_bufferManager;
};

TEST(VariableView, copy_and_view) {
  VariableView<double> v(4);

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
