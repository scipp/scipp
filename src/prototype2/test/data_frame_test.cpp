#include <gtest/gtest.h>

#include "data_frame.h"
#include "unit.h"
#include "vectors.h"

using Doubles = std::vector<double>;
using Int32s = std::vector<int32_t>;
using Int64s = std::vector<int64_t>;

TEST(DataFrame, construct_empty) {
  DataFrame<Doubles, Int32s, Int64s> f;
  ASSERT_EQ(f.size(), 0);
}

TEST(DataFrame, construct) {
  DataFrame<Doubles, Int32s, Int64s> f({1.0}, {"a", "b"}, {3}, {4ll});
  ASSERT_EQ(f.size(), 1);
}

TEST(DataFrame, get) {
  DataFrame<Doubles, Int32s, Int64s> f({1.0, 2.0}, {"a", "b"}, {3, 7},
                                       {4ll, 5ll});
  ASSERT_EQ(f.size(), 2);
  // Note that using get<T> will not compile if there are duplicate types in the
  // DataFrame.
  ASSERT_EQ(f.get<Int32s>(), (Int32s{3, 7}));
  ASSERT_EQ(f.get<Int64s>(), (Int64s{4ll, 5ll}));
}

using Histogram = DataFrame<BinEdges<Unit::Tof>, Counts<>, CountStdDevs<>>;

TEST(DataFrame, construct_histogram) {
  Histogram h({1.0, 2.0}, {3, 7}, {4, 5});
  ASSERT_EQ(h.size(), 2);
}

TEST(DataFrame, construct_nested) {
  Histogram h({1.0, 2.0}, {3, 7}, {4, 5});
  using SpectrumNumber = int32_t;
  using SpectrumInfo = std::vector<char>; // dummy
  using Histograms = std::vector<Histogram>;
  DataFrame<std::vector<SpectrumNumber>, Histograms, SpectrumInfo> f({1}, {h},
                                                                     {'x'});
  ASSERT_EQ(f.size(), 1);
  for (const auto &hist : f.get<Histograms>())
    ASSERT_EQ(hist.get<Counts<>>()[0], 3);
}
