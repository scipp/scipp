#include <gtest/gtest.h>

#include "dataset.h"

TEST(Dataset, construct_empty) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, construct) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, columns) {
  Dataset d;
  d.add<Variable::Value>("name1");
  d.add<Variable::Int>("name2");
  ASSERT_EQ(d.size(), 2);
}

TEST(Dataset, extendAlongDimension) {
  Dataset d;
  d.add<Variable::Value>("name1");
  d.add<Variable::Int>("name2");
  d.addDimension(Dimension::Tof, 10);
  d.extendAlongDimension<Variable::Value>(Dimension::Tof);
}

TEST(Dataset, get) {
  Dataset d;
  d.add<Variable::Value>("name1");
  d.add<Variable::Int>("name2");
  auto &view = d.get<Variable::Value>();
  ASSERT_EQ(view.size(), 1);
  view[0] = 1.2;
  ASSERT_EQ(view[0], 1.2);
}

TEST(Dataset, get_const) {
  Dataset d;
  d.add<Variable::Value>("name1");
  d.add<Variable::Int>("name2");
  auto &view = d.get<const Variable::Value>();
  ASSERT_EQ(view.size(), 1);
  // auto is now deduced to be const, so assignment will not compile:
  // view[0] = 1.2;
}

TEST(Dataset, view_tracks_changes) {
  Dataset d;
  d.add<Variable::Value>("name1");
  d.add<Variable::Int>("name2");
  auto &view = d.get<Variable::Value>();
  ASSERT_EQ(view.size(), 1);
  view[0] = 1.2;
  d.addDimension(Dimension::Tof, 3);
  d.extendAlongDimension<Variable::Value>(Dimension::Tof);
  ASSERT_EQ(view.size(), 3);
  EXPECT_EQ(view[0], 1.2);
  EXPECT_EQ(view[1], 0.0);
  EXPECT_EQ(view[2], 0.0);
}
