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

TEST(Dataset, ragged_dimension) {
  Dataset d;
  d.add<Variable::Value>("data");
  // Use a special reserved name here?
  // Do we need a name? Only if we support multiple ragged dimensions?
  d.add<Variable::DimensionSize>("bin_count");
  d.addDimension(Dimension::SpectrumNumber, 3);
  d.addDimension(Dimension::Tof, "bin_count");
  d.extendAlongDimension<Variable::DimensionSize>(Dimension::SpectrumNumber);
  // TODO prevent changing this after creation, need to do everything in a single call.
  auto &binCounts = d.get<Variable::DimensionSize>();
  binCounts[0] = 7;
  binCounts[1] = 4;
  binCounts[2] = 1;
  d.extendAlongDimension<Variable::Value>(Dimension::Tof);
  auto &view = d.get<Variable::Value>();
  ASSERT_EQ(view.size(), 7 + 4 + 1);
  const auto &dims = d.dimensions<Variable::DimensionSize>();
  ASSERT_EQ(dims.size(), 1);
  EXPECT_EQ(dims[0], Dimension::SpectrumNumber);

  // What should a good API look like?
  // Do not manually add dimensions? Empty dimension is pointless? OR is it?
  // Works only if dimensions exist in d already, otherwise length is unknown
  //d.add<Variable::Value>("data", {Dimension::Tof, Dimension::Spectrum});

  // Should we only support this for histograms and use size from within Variable::Histogram?
  // d.add<Variable::Histogram>("sample", <size information>);
  // Would people want to use it for different things, such as a different number of temperature points in each run?
  // Use equivalent to xarray.DataArray to define dimensionality?
  // store name, dimensions, and dimension extends in Variable (currently ColumnHandle)?
  //
  // Edges: Just use a variable with a longer dimension (require special setter on Dataset)?
  // Are edges *always* corresponding 1:1 to a dimension?
  // - Dimension::Tof -> Variable::Tof?
  // Edges only make sense for dimensions (otherwise it would just be labels, i.e., data in arbitrary order)!
  // ?? No, only for contiguous dimensions, Dimension::Spectrum ist just an int label. but still a dimension.
  // how can we nicely match variable labels and dimension labels?
  //Variable qEdges(Variable::Q, Dimension::Q, Dimension::DeltaE);
  // Variable::Q Dimension::Q feels redundant, we are simply defining the independent variable Q?
  //Variable intensity(Variable::Value, Dimension::Q, Dimension::DeltaE);

}
