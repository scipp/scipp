#include <gtest/gtest.h>

#include "dataset.h"

TEST(Dataset, construct_empty) {
  ASSERT_NO_THROW(Dataset d);
}

TEST(Dataset, construct) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, columns) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  ASSERT_EQ(d.columns(), 2);
}

TEST(Dataset, extendAlongDimension) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  d.addDimension(Dimension::Tof, 10);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
}

TEST(Dataset, get) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  auto &view = d.get<Doubles>();
  ASSERT_EQ(view.size(), 1);
  view[0] = 1.2;
  ASSERT_EQ(view[0], 1.2);
}

TEST(Dataset, view_tracks_changes) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  auto &view = d.get<Doubles>();
  ASSERT_EQ(view.size(), 1);
  view[0] = 1.2;
  d.addDimension(Dimension::Tof, 3);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  ASSERT_EQ(view.size(), 3);
  EXPECT_EQ(view[0], 1.2);
  EXPECT_EQ(view[1], 0.0);
  EXPECT_EQ(view[2], 0.0);
}
