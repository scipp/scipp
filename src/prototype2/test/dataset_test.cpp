#include <gtest/gtest.h>

#include "dataset.h"

TEST(Dataset, construct_empty) {
  std::array<std::vector<Dimension>, 2> dimensions;
  dimensions[0].push_back(Dimension::SpectrumNumber);
  Dataset<std::vector<int>, std::vector<double>> s(
      dimensions, std::vector<int>{}, std::vector<double>{});
  EXPECT_ANY_THROW(s.size(Dimension::Tof));
  EXPECT_EQ(s.size(Dimension::SpectrumNumber), 0);
}

TEST(FlatDataset, addDimension) {
  FlatDataset<int, double, char> s;
  ASSERT_NO_THROW(s.addDimension("Spectrum", 10));
  ASSERT_NO_THROW(s.addDimension("Tof", 5));
  EXPECT_EQ(s.get<int>().size(), 1);
  EXPECT_EQ(s.get<double>().size(), 1);
  EXPECT_EQ(s.get<char>().size(), 1);
}

TEST(FlatDataset, extendAlongDimension) {
  FlatDataset<int, double, char> s;
  s.addDimension("Spectrum", 10);
  s.addDimension("Tof", 5);
  s.extendAlongDimension<double>("Tof");
  EXPECT_EQ(s.get<int>().size(), 1);
  EXPECT_EQ(s.get<double>().size(), 5);
  EXPECT_EQ(s.get<char>().size(), 1);
  s.extendAlongDimension<int>("Spectrum");
  s.extendAlongDimension<double>("Spectrum");
  EXPECT_EQ(s.get<int>().size(), 10);
  EXPECT_EQ(s.get<double>().size(), 50);
  EXPECT_EQ(s.get<char>().size(), 1);
  // Extend again along *same dimension* (could be useful for correlations?)
  s.extendAlongDimension<double>("Tof");
  EXPECT_EQ(s.get<int>().size(), 10);
  EXPECT_EQ(s.get<double>().size(), 250);
  EXPECT_EQ(s.get<char>().size(), 1);
}
