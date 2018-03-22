#include <gtest/gtest.h>

#include "dataset_iterator.h"

TEST(DatasetIterator, construct) {
  Dataset d(std::vector<double>(1), std::vector<int>(1));
  ASSERT_NO_THROW(DatasetIterator<> it(d));
  ASSERT_NO_THROW(DatasetIterator<double> it(d));
  ASSERT_NO_THROW(DatasetIterator<int> it(d));
  ASSERT_NO_THROW(auto it = (DatasetIterator<int, double>(d)));
  ASSERT_THROW(auto it = (DatasetIterator<int, float>(d)), std::runtime_error);
}

TEST(DatasetIterator, get) {
  Dataset d(std::vector<double>(1), std::vector<int>(1));
  d.addDimension("tof", 10);
  d.extendAlongDimension(ColumnType::Doubles, "tof");
  d.extendAlongDimension(ColumnType::Ints, "tof");
  auto &view = d.get<Doubles>();
  view[0] = 0.2;
  view[3] = 3.2;

  DatasetIterator<double> it(d, 0);
  ASSERT_EQ(it.get<double>(), 0.2);
  it.increment();
  ASSERT_EQ(it.get<double>(), 0.0);
  it.increment();
  ASSERT_EQ(it.get<double>(), 0.0);
  it.increment();
  ASSERT_EQ(it.get<double>(), 3.2);
}
