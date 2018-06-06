#include <gtest/gtest.h>
#include <vector>

#include "data_array.h"
#include "variable.h"

TEST(DataArray, construct) {
  ASSERT_NO_THROW(makeDataArray<Variable::Value>(2));
  const auto a = makeDataArray<Variable::Value>(2);
  const auto &data = a.get<Variable::Value>();
  EXPECT_EQ(data.size(), 2);
}

TEST(DataArray, sharing) {
  const auto a1 = makeDataArray<Variable::Value>(2);
  const auto a2(a1);
  EXPECT_EQ(&a1.get<Variable::Value>(), &a2.get<Variable::Value>());
}

TEST(DataArray, copy) {
  const auto a1 = makeDataArray<Variable::Value>({1.1, 2.2});
  const auto &data1 = a1.get<Variable::Value>();
  EXPECT_EQ(data1[0], 1.1);
  EXPECT_EQ(data1[1], 2.2);
  auto a2(a1);
  EXPECT_EQ(&a1.get<Variable::Value>(), &a2.get<const Variable::Value>());
  EXPECT_NE(&a1.get<Variable::Value>(), &a2.get<Variable::Value>());
  const auto &data2 = a2.get<Variable::Value>();
  EXPECT_EQ(data2[0], 1.1);
  EXPECT_EQ(data2[1], 2.2);
}
