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
  auto &view = d.get<double>();
  ASSERT_EQ(view.size(), 1);
  view[0] = 1.2;
  ASSERT_EQ(view[0], 1.2);
}

TEST(Dataset, view_tracks_changes) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  auto &view = d.get<double>();
  ASSERT_EQ(view.size(), 1);
  view[0] = 1.2;
  d.addDimension(Dimension::Tof, 3);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  ASSERT_EQ(view.size(), 3);
  EXPECT_EQ(view[0], 1.2);
  EXPECT_EQ(view[1], 0.0);
  EXPECT_EQ(view[2], 0.0);
}

#if 0
TEST(Dataset, histogram_view) {
  // bin edges, values, and errors.
  Dataset d(std::vector<double>(1), std::vector<double>(1),
            std::vector<double>(1));
  d.addDimension("spectrum", 3);
  // TODO ColumnId?
  d.extendAlongDimension(ColumnId::BinEdges, "spectrum"); // optional
  d.extendAlongDimension(ColumnId::Values, "spectrum");
  d.extendAlongDimension(ColumnId::Errors, "spectrum");
  // How to handle non-const fields that have extra dimensions? would need to
  // have extra index/stride access? but type would be different, i.e., we need
  // different client code, handling different cases!
  // Can we handle BinEdges/Values/Errors with units?
  auto &view =
      d.get<const BinEdges, Values, Errors>(); // provide getters/setters in
                                               // view via mixins? should we
                                               // handle fields with less or
                                               // more dimensions here?
  // if all data in flat array, how can we distinguish iterations over
  // individual values vs. full histograms? use a special helper type
  // `Histograms` when getting the view?
  for(auto &histogram : view) {
    // might want to access also SpectrumInfo etc.
    histogram *= 2.0;
  }

  // access to BinEdges, Values, Counts, position, masking?
  // exploding number of types, or manageable?
  // - SpectrumInfo, QInfo
  // - SpectrumNumber?
  // - BinMasking?
  // - const/non-const BinEdges/Points
  // - always Values and Errors?
  SpectrumView<const BinEdges, Values, Errors, SpectrumInfo>
      view; // could request non-const Binedges, throw if shared?
  for(auto &spectrum : view) {

  }

  // Is TOF-slice access really performance relevant?

  // does slice return by value, or reference with stride access?
  delta = d.slice<Polarization>("up") - d.slice<Polarization>("down");

  d.spectra(); // special slicing case? still type-erased? can we offset the cost?
  d.spectra().get<const BinEdges, Values, Errors>();

  // need to specify: contained columns, column used for iteration? Suffers from same dimensionality issue?
  // slice => all items are vectors (i.e., just return a new Dataset)?
  // - ok as long as we do not want to process slices
  // - not ok if processing is needed, since we still need to cast types
  // 
  d.at<Value>(i);
  auto &slice = d.slice("spectrum", 2); // what does this return? performance implications of converting to view after slicing?
  for(auto &slice : d.slices("spectrum")) {
    // slice is still type-erased, can we avoid the cost of casting for small slices?
  }
  auto &view = d.get<const BinEdges, Value, Error>;
  // this is maybe not so useful, item would contain extra dimensions such as polarization.
  for(auto &item : view.slice("spectrum")) { // how can this work if view does not contain spectrum axis?
    //
  }

  // similar to view?? view provides iteration over 1 or more columns, slice is access

  //auto &view = d.slice<Polarization>.get<const BinEdges, Values, Errors>();
  //auto &slice = d.slice<const BinEdges, Values, Errors>(polarization);
}

TEST(Dataset, example) {
  Dataset d(std::vector<double>(1), std::vector<double>(1),
            std::vector<double>(1));
}
#endif
