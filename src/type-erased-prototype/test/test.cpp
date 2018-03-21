#include <gtest/gtest.h>

#include "dataset.h"

TEST(Dataset, construct_empty) {
  ASSERT_NO_THROW(Dataset d);
}

TEST(Dataset, construct) {
  // TODO different construction mechanism that does not require passing length
  // 1 vectors.
  ASSERT_NO_THROW(Dataset d(std::vector<double>(1), std::vector<int>(1)));
}

TEST(Dataset, columns) {
  Dataset d(std::vector<double>(1), std::vector<int>(1));
  ASSERT_EQ(d.columns(), 2);
}

TEST(Dataset, extendAlongDimension) {
  Dataset d(std::vector<double>(1), std::vector<int>(1));
  d.addDimension("tof", 10);
  d.extendAlongDimension(ColumnType::Doubles, "tof");
}

TEST(Dataset, get) {
  Dataset d(std::vector<double>(1), std::vector<int>(1));
  auto &view = d.get<Doubles>();
  ASSERT_EQ(view.size(), 1);
  view[0] = 1.2;
  ASSERT_EQ(view[0], 1.2);
}

TEST(Dataset, view_tracks_changes) {
  Dataset d(std::vector<double>(1), std::vector<int>(1));
  auto &view = d.get<Doubles>();
  ASSERT_EQ(view.size(), 1);
  view[0] = 1.2;
  d.addDimension("tof", 3);
  d.extendAlongDimension(ColumnType::Doubles, "tof");
  ASSERT_EQ(view.size(), 3);
  EXPECT_EQ(view[0], 1.2);
  EXPECT_EQ(view[1], 0.0);
  EXPECT_EQ(view[2], 0.0);
}
