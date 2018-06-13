#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset.h"
#include "dimensions.h"

TEST(Dataset, construct_empty) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, construct) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, insert_coords) {
  Dataset d;
  d.insert<Coord::Tof>(Dimensions{}, {1.1});
  d.insert<Coord::SpectrumNumber>(Dimensions{}, {2});
  EXPECT_THROW_MSG(d.insert<Coord::SpectrumNumber>(Dimensions{}, {2}),
                   std::runtime_error,
                   "Attempt to insert duplicate coordinate.");
  ASSERT_EQ(d.size(), 2);
}

TEST(Dataset, insert_data) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
  EXPECT_THROW_MSG(d.insert<Data::Int>("name2", Dimensions{}, {2l}),
                   std::runtime_error,
                   "Attempt to insert data of same type with duplicate name.");
  ASSERT_EQ(d.size(), 2);
}

TEST(Dataset, insert_variables_with_dimensions) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
}

TEST(Dataset, insert_variables_dimension_fail) {
  Dimensions xy;
  Dimensions xz;
  Dimensions yz;
  xy.add(Dimension::X, 1);
  xz.add(Dimension::X, 1);
  xy.add(Dimension::Y, 2);
  yz.add(Dimension::Y, 2);
  xz.add(Dimension::Z, 3);
  yz.add(Dimension::Z, 3);
  Dataset xyz;
  xyz.insert<Data::Value>("name1", xy, 2);
  EXPECT_NO_THROW(xyz.insert<Data::Value>("name2", yz, 6));
  EXPECT_NO_THROW(xyz.insert<Data::Value>("name3", xz, 3));
  // The following should also work (and NOT throw), it is simply constructing
  // the same Dataset in a different order. For the time being, it does NOT
  // work, but this is simply due to a crude preliminary implementation of the
  // dimension merging code in Dataset.
  Dataset xzy;
  xzy.insert<Data::Value>("name1", xz, 3);
  EXPECT_NO_THROW(xzy.insert<Data::Value>("name2", xy, 2));
  EXPECT_THROW_MSG(
      xzy.insert<Data::Value>("name3", yz, 6), std::runtime_error,
      "Cannot insert variable into Dataset: Dimension order mismatch");
}

TEST(Dataset, insertAsEdge) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  auto edges = makeDataArray<Data::Error>(Dimensions(Dimension::Tof, 3),
                                          {1.1, 2.2, 3.3});
  edges.setName("edges");
  EXPECT_EQ(d.dimensions().size(Dimension::Tof), 2);
  EXPECT_THROW_MSG(
      d.insert(edges), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
  EXPECT_NO_THROW(d.insertAsEdge(Dimension::Tof, edges));
}

TEST(Dataset, insertAsEdge_fail) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  auto too_short =
      makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  EXPECT_THROW_MSG(
      d.insertAsEdge(Dimension::Tof, too_short), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
  auto too_long = makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 4),
                                             {1.1, 2.2, 3.3, 4.4});
  EXPECT_THROW_MSG(
      d.insertAsEdge(Dimension::Tof, too_long), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
  auto edges = makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 3),
                                          {1.1, 2.2, 3.3});
  EXPECT_THROW_MSG(d.insertAsEdge(Dimension::X, edges), std::runtime_error,
                   "Dimension not found.");
}

TEST(Dataset, insertAsEdge_reverse_fail) {
  Dataset d;
  auto edges =
      makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  d.insertAsEdge(Dimension::Tof, edges);
  EXPECT_THROW_MSG(
      d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2}),
      std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
}

TEST(Dataset, get) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
  auto &view = d.get<Data::Value>();
  ASSERT_EQ(view.size(), 1);
  EXPECT_EQ(view[0], 1.1);
  view[0] = 2.2;
  EXPECT_EQ(view[0], 2.2);
}

TEST(Dataset, get_const) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
  auto &view = d.get<const Data::Value>();
  ASSERT_EQ(view.size(), 1);
  EXPECT_EQ(view[0], 1.1);
  // auto is now deduced to be const, so assignment will not compile:
  // view[0] = 1.2;
}

TEST(Dataset, get_fail) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Value>("name2", Dimensions{}, {1.1});
  EXPECT_THROW_MSG(d.get<Data::Value>(), std::runtime_error,
                   "Given variable tag is not unique. Must provide a name.");
  EXPECT_THROW_MSG(d.get<Data::Int>(), std::runtime_error,
                   "Dataset does not contain such a variable.");
}

TEST(Dataset, get_named) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Value>("name2", Dimensions{}, {2.2});
  auto &var1 = d.get<Data::Value>("name1");
  ASSERT_EQ(var1.size(), 1);
  EXPECT_EQ(var1[0], 1.1);
  auto &var2 = d.get<Data::Value>("name2");
  ASSERT_EQ(var2.size(), 1);
  EXPECT_EQ(var2[0], 2.2);
}

#if 0
TEST(Dataset, ragged_dimension) {
  Dataset d;
  d.add<Data::Value>("data");
  // Use a special reserved name here?
  // Do we need a name? Only if we support multiple ragged dimensions?
  d.add<Data::DimensionSize>("bin_count");
  d.addDimension(Dimension::SpectrumNumber, 3);
  d.addDimension(Dimension::Tof, "bin_count");
  d.extendAlongDimension<Data::DimensionSize>(Dimension::SpectrumNumber);
  // TODO prevent changing this after creation, need to do everything in a single call.
  auto &binCounts = d.get<Data::DimensionSize>();
  binCounts[0] = 7;
  binCounts[1] = 4;
  binCounts[2] = 1;
  d.extendAlongDimension<Data::Value>(Dimension::Tof);
  auto &view = d.get<Data::Value>();
  ASSERT_EQ(view.size(), 7 + 4 + 1);
  const auto &dims = d.dimensions<Data::DimensionSize>();
  ASSERT_EQ(dims.size(), 1);
  EXPECT_EQ(dims[0], Dimension::SpectrumNumber);

  // What should a good API look like?
  // Do not manually add dimensions? Empty dimension is pointless? OR is it?
  // Works only if dimensions exist in d already, otherwise length is unknown
  //d.add<Data::Value>("data", {Dimension::Tof, Dimension::Spectrum});

  // Should we only support this for histograms and use size from within Data::Histogram?
  // d.add<Data::Histogram>("sample", <size information>);
  // Would people want to use it for different things, such as a different number of temperature points in each run?
  // Use equivalent to xarray.DataArray to define dimensionality?
  // store name, dimensions, and dimension extends in Data (currently ColumnHandle)?
  //
  // Edges: Just use a variable with a longer dimension (require special setter on Dataset)?
  // Are edges *always* corresponding 1:1 to a dimension?
  // - Dimension::Tof -> Data::Tof?
  // Edges only make sense for dimensions (otherwise it would just be labels, i.e., data in arbitrary order)!
  // ?? No, only for contiguous dimensions, Dimension::Spectrum ist just an int label. but still a dimension.
  // how can we nicely match variable labels and dimension labels?
  //Data qEdges(Data::Q, Dimension::Q, Dimension::DeltaE);
  // Data::Q Dimension::Q feels redundant, we are simply defining the independent variable Q?
  //Data intensity(Data::Value, Dimension::Q, Dimension::DeltaE);
}
#endif
