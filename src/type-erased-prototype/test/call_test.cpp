#include <gtest/gtest.h>

#include "call_wrappers.h"
#include "dataset.h"

struct DatasetAlgorithm {
  void apply(Dataset &dataset) const {}
};

struct ColumnAlgorithm {
  void apply(Doubles &column) const {}
  void operator()(Doubles &column) const {}
};

struct SingleItemAlgorithm {
  static void apply(double &x) { x += 1.5; }
};

struct TwoItemAlgorithm {
  static void apply(double &x, int &i) { x *= i; }
};

TEST(CallWrappers, call_DatasetAlgorithm) {
  Dataset d;
  d = call<DatasetAlgorithm>(std::move(d));
}

TEST(CallWrappers, call_SingleItemAlgorithm) {
  Dataset d;
  d.addColumn<double>("name1");
  d = call<SingleItemAlgorithm>(std::move(d));
  ASSERT_EQ(d.get<Doubles>()[0], 1.5);
}

TEST(CallWrappers, call_TwoItemAlgorithm) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  d.get<Ints>()[0] = 2;
  d = call<SingleItemAlgorithm>(std::move(d));
  d = call<TwoItemAlgorithm>(std::move(d));
  ASSERT_EQ(d.get<Doubles>()[0], 3.0);
}

/*
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
*/
