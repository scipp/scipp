#include <gtest/gtest.h>

#include "dataset_iterator.h"

TEST(DatasetIterator, construct) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  ASSERT_NO_THROW(DatasetIterator<> it(d));
  ASSERT_NO_THROW(DatasetIterator<double> it(d));
  ASSERT_NO_THROW(DatasetIterator<int> it(d));
  ASSERT_NO_THROW(auto it = (DatasetIterator<int, double>(d)));
  ASSERT_THROW(auto it = (DatasetIterator<int, float>(d)), std::runtime_error);
}

TEST(DatasetIterator, single_column) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  d.addDimension(Dimension::Tof, 10);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  d.extendAlongDimension(ColumnType::Ints, Dimension::Tof);
  auto &view = d.get<Doubles>();
  view[0] = 0.2;
  view[3] = 3.2;

  DatasetIterator<double> it(d);
  ASSERT_EQ(it.get<double>(), 0.2);
  it.increment();
  ASSERT_EQ(it.get<double>(), 0.0);
  it.increment();
  ASSERT_EQ(it.get<double>(), 0.0);
  it.increment();
  ASSERT_EQ(it.get<double>(), 3.2);
}

TEST(DatasetIterator, multi_column) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  d.addDimension(Dimension::Tof, 2);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  d.extendAlongDimension(ColumnType::Ints, Dimension::Tof);
  auto &view = d.get<Doubles>();
  view[0] = 0.2;
  view[1] = 3.2;

  DatasetIterator<double, int> it(d);
  ASSERT_EQ(it.get<double>(), 0.2);
  ASSERT_EQ(it.get<int>(), 0);
  it.increment();
  ASSERT_EQ(it.get<double>(), 3.2);
  ASSERT_EQ(it.get<int>(), 0);
}

TEST(DatasetIterator, multi_column_mixed_dimension) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  d.addDimension(Dimension::Tof, 2);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  auto &view = d.get<Doubles>();
  view[0] = 0.2;
  view[1] = 3.2;

  ASSERT_ANY_THROW(auto it = (DatasetIterator<double, int>(d)));
  ASSERT_NO_THROW(auto it = (DatasetIterator<double, const int>(d)));
  auto it = (DatasetIterator<double, const int>(d));
  ASSERT_EQ(it.get<double>(), 0.2);
  ASSERT_EQ(it.get<const int>(), 0);
  it.increment();
  ASSERT_EQ(it.get<double>(), 3.2);
  ASSERT_EQ(it.get<const int>(), 0);
}

TEST(DatasetIterator, multi_column_mixed_dimension_with_slab) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  d.addDimension(Dimension::Tof, 2);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  auto &view = d.get<Doubles>();
  view[0] = 0.2;
  view[1] = 3.2;

  // Should fixed dimension be generic, or should we just provide a couple of special cases, in particular for Tof?
  // Use direct column access otherwise (how to access things like Tof slices?)?
  // YAGNI? Just support a single fixed dimension, given at compile time?!
  // We might want to iterate all BinEdges, getting a slab of corresponding
  // counts (e.g., all spectra for certain polarization)?! That is,
  // Dimension::Tof and Dimension::SpectrumNumber might be fixed, while
  // Dimension::Polarization is running. Or Dimension::Tof and
  // Dimension::Polarization are fixed file Dimension::SpectrumNumber is
  // running. The latter is more likely to be expressed as "iterate only
  // SpectrumNumber", how do we handle that in general?
  // Maybe a better way to say this is: Iterate all dimensions of BinEdges. In
  // general we do not know which other columns need to be accessed as slabs,
  // how can we deal with this? Just access all as slab (which may be size 1)?
  DatasetIterator<Slab<double>, int> it(d, {Dimension::Tof});
  // it.get<double>(); // Does not compile, since we cannot get a single double.
  it.get<int>();
}

#if 0
TEST(DatasetIterator, notes) {
  Dataset d(std::vector<double>(1), std::vector<int>(1));
  d.addDimension("tof", 10);
  d.extendAlongDimension(ColumnType::Doubles, "tof");
  d.addDimension("spec", 10);
  d.extendAlongDimension(ColumnType::Doubles, "spec");
  d.extendAlongDimension(ColumnType::Ints, "spec");

  // ok
  DatasetIterator<double> it(d, 0);
  // should throw, because int has less dimensions and is not const
  DatasetIterator<double, int> it(d, 0);
  // ok
  DatasetIterator<double, const int> it(d, 0);
  // ok, int can be non-const since slab says "do not iterate tof".
  // This is a common case so we may want a shorthand notation for this.
  // We also need a way to have multiple columns of the same type, X,Y,E of a
  // histogram are all double currently! If we have a type for, e.g., BinEdges
  // wrapping a std::vector<double> we can use that (but template argument would
  // not be item type anymore!). Can we use a column ID instead? Since the
  // iterator is templated we cannot support custom types anyway.
  // Unit handling?? Can/should a slab have a (runtime) unit? Could used by call
  // wrapper to create output workspace (cannot *set* based on it due to
  // conflict with multi threading, but can *check*)? (do we even need call
  // wrappers, or should things be handled based on Dataset and
  // transformations?).
  DatasetIterator<slab<double, Dimension::Tof>, int> it(d, 0);
  // iterate over items that are slabs of doubles with a spectrum dimension. In
  // this case iteration is over tof.
  // A slab is a bit like gsl::range, i.e., a view into a vector but may have a
  // spread to support multi-dimensional slices.
  DatasetIterator<slab<double, Dimension::Spectrum>,
                  slab<const int, Dimension::Spectrum>> it(d, 0);
}
#endif
