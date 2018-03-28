#include <gtest/gtest.h>

#include "data_frame_view.h"

TEST(TupleView, test) {
  std::tuple<int, double, char> t(7, 3.14, 'c');
  auto view = makeTupleView<double, const char>(t);

  EXPECT_EQ(std::get<0>(view), 3.14);

  std::get<0>(view) = 1.2;
  EXPECT_EQ(std::get<1>(t), 1.2);

  // std::get<1>(view) = 'd'; // const, does not compile
  std::get<2>(t) = 'd';
  EXPECT_EQ(std::get<1>(view), 'd');
}
