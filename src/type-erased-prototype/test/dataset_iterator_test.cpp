#include <gtest/gtest.h>

#include <boost/mpl/at.hpp>
#include <boost/mpl/sort.hpp>
#include <boost/mpl/vector_c.hpp>

#include "test_macros.h"

#include "dataset_iterator.h"

TEST(MultidimensionalIndex, end) {
  const std::vector<gsl::index> volume{3, 1, 2};
  MultidimensionalIndex i(volume);
  ASSERT_EQ(i.end, (std::vector<gsl::index>{2, 0, 1}));
}

TEST(MultidimensionalIndex, increment) {
  const std::vector<gsl::index> volume{3, 1, 2};
  MultidimensionalIndex i(volume);
  ASSERT_EQ(i.index, (std::vector<gsl::index>{0, 0, 0}));
  i.increment();
  ASSERT_EQ(i.index, (std::vector<gsl::index>{1, 0, 0}));
  i.increment();
  ASSERT_EQ(i.index, (std::vector<gsl::index>{2, 0, 0}));
  i.increment();
  ASSERT_EQ(i.index, (std::vector<gsl::index>{0, 0, 1}));
  i.increment();
  ASSERT_EQ(i.index, (std::vector<gsl::index>{1, 0, 1}));
  i.increment();
  ASSERT_EQ(i.index, (std::vector<gsl::index>{2, 0, 1}));
}

TEST(LinearSubindex, full_subindex) {
  const std::vector<gsl::index> volume{3, 1, 2};
  const std::map<Dimension, gsl::index> extents{
      {Dimension::Tof, 3}, {Dimension::SpectrumNumber, 1}, {Dimension::Q, 2}};
  MultidimensionalIndex i(volume);
  Dimensions dims;
  dims.add(Dimension::Tof, 3);
  dims.add(Dimension::SpectrumNumber, 1);
  dims.add(Dimension::Q, 2);

  LinearSubindex sub(extents, dims, i);
  gsl::index count{0};
  while (true) {
    ASSERT_EQ(sub.get(), count++);
    if (i.index == i.end)
      break;
    i.increment();
  }
}

TEST(LinearSubindex, zero_dimensional_subindex) {
  const std::vector<gsl::index> volume{3, 1, 2};
  const std::map<Dimension, gsl::index> extents{
      {Dimension::Tof, 3}, {Dimension::SpectrumNumber, 1}, {Dimension::Q, 2}};
  MultidimensionalIndex i(volume);

  LinearSubindex sub(extents, Dimensions{}, i);
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 0);
}

TEST(LinearSubindex, fast_1_dimensional_subindex) {
  const std::vector<gsl::index> volume{3, 1, 2};
  const std::map<Dimension, gsl::index> extents{
      {Dimension::Tof, 3}, {Dimension::SpectrumNumber, 1}, {Dimension::Q, 2}};
  MultidimensionalIndex i(volume);

  LinearSubindex sub(extents, Dimensions(Dimension::Tof, 3), i);
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 1);
  i.increment();
  ASSERT_EQ(sub.get(), 2);
  i.increment();
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 1);
  i.increment();
  ASSERT_EQ(sub.get(), 2);
}

TEST(LinearSubindex, slow_1_dimensional_subindex) {
  const std::vector<gsl::index> volume{3, 1, 2};
  const std::map<Dimension, gsl::index> extents{
      {Dimension::Tof, 3}, {Dimension::SpectrumNumber, 1}, {Dimension::Q, 2}};
  MultidimensionalIndex i(volume);

  LinearSubindex sub(extents, Dimensions(Dimension::Q, 2), i);
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 1);
  i.increment();
  ASSERT_EQ(sub.get(), 1);
  i.increment();
  ASSERT_EQ(sub.get(), 1);
}

TEST(LinearSubindex, flipped_2_dimensional_subindex) {
  const std::vector<gsl::index> volume{3, 1, 2};
  const std::map<Dimension, gsl::index> extents{
      {Dimension::Tof, 3}, {Dimension::SpectrumNumber, 1}, {Dimension::Q, 2}};
  MultidimensionalIndex i(volume);
  Dimensions dims;
  dims.add(Dimension::Q, 2);
  dims.add(Dimension::Tof, 3);

  LinearSubindex sub(extents, dims, i);
  ASSERT_EQ(sub.get(), 0);
  i.increment();
  ASSERT_EQ(sub.get(), 2);
  i.increment();
  ASSERT_EQ(sub.get(), 4);
  i.increment();
  ASSERT_EQ(sub.get(), 1);
  i.increment();
  ASSERT_EQ(sub.get(), 3);
  i.increment();
  ASSERT_EQ(sub.get(), 5);
}

TEST(DatasetIterator, construct) {
  Dataset d;
  d.insert<Variable::Value>("name1", Dimensions{}, {1.1});
  d.insert<Variable::Int>("name2", Dimensions{}, {2l});
  ASSERT_NO_THROW(DatasetIterator<> it(d));
  ASSERT_NO_THROW(DatasetIterator<Variable::Value> it(d));
  ASSERT_NO_THROW(DatasetIterator<Variable::Int> it(d));
  ASSERT_NO_THROW(auto it =
                      (DatasetIterator<Variable::Int, Variable::Value>(d)));
  ASSERT_THROW(auto it = (DatasetIterator<Variable::Int, Variable::Error>(d)),
               std::runtime_error);
}

TEST(DatasetIterator, single_column) {
  Dataset d;
  d.insert<Variable::Value>("name1", Dimensions(Dimension::Tof, 10), 10);
  d.insert<Variable::Int>("name2", Dimensions(Dimension::Tof, 10), 10);
  auto &view = d.get<Variable::Value>();
  view[0] = 0.2;
  view[3] = 3.2;

  DatasetIterator<Variable::Value> it(d);
  ASSERT_EQ(it.get<Variable::Value>(), 0.2);
  it.increment();
  ASSERT_EQ(it.get<Variable::Value>(), 0.0);
  it.increment();
  ASSERT_EQ(it.get<Variable::Value>(), 0.0);
  it.increment();
  ASSERT_EQ(it.get<Variable::Value>(), 3.2);
}

TEST(DatasetIterator, multi_column) {
  Dataset d;
  d.insert<Variable::Value>("name1", Dimensions(Dimension::Tof, 2), 2);
  d.insert<Variable::Int>("name2", Dimensions(Dimension::Tof, 2), 2);
  auto &view = d.get<Variable::Value>();
  view[0] = 0.2;
  view[1] = 3.2;

  DatasetIterator<Variable::Value, Variable::Int> it(d);
  ASSERT_EQ(it.get<Variable::Value>(), 0.2);
  ASSERT_EQ(it.get<Variable::Int>(), 0);
  it.increment();
  ASSERT_EQ(it.get<Variable::Value>(), 3.2);
  ASSERT_EQ(it.get<Variable::Int>(), 0);
}

TEST(DatasetIterator, multi_column_mixed_dimension) {
  Dataset d;
  d.insert<Variable::Value>("name1", Dimensions(Dimension::Tof, 2), 2);
  d.insert<Variable::Int>("name2", Dimensions{}, 1);
  auto &view = d.get<Variable::Value>();
  view[0] = 0.2;
  view[1] = 3.2;

  ASSERT_ANY_THROW(auto it =
                       (DatasetIterator<Variable::Value, Variable::Int>(d)));
  ASSERT_NO_THROW(
      auto it = (DatasetIterator<Variable::Value, const Variable::Int>(d)));
  auto it = (DatasetIterator<Variable::Value, const Variable::Int>(d));
  ASSERT_EQ(it.get<Variable::Value>(), 0.2);
  ASSERT_EQ(it.get<const Variable::Int>(), 0);
  it.increment();
  ASSERT_EQ(it.get<Variable::Value>(), 3.2);
  ASSERT_EQ(it.get<const Variable::Int>(), 0);
}

TEST(DatasetIterator, multi_column_unrelated_dimension) {
  Dataset d;
  d.insert<Variable::Value>("name1", Dimensions(Dimension::X, 2), 2);
  d.insert<Variable::Int>("name2", Dimensions(Dimension::Y, 3), 3);
  DatasetIterator<Variable::Value> it(d);
  it.increment();
  // We iterate only Variable::Value, so there should be no iteration in
  // Dimension::Y.
  ASSERT_TRUE(it.atLast());
}

TEST(DatasetIterator, multi_column_mixed_dimension_with_slab) {
  Dataset d;
  d.insert<Variable::Value>("name1", Dimensions(Dimension::Tof, 2), 2);
  d.insert<Variable::Int>("name2", Dimensions{}, 1);
  auto &view = d.get<Variable::Value>();
  view[0] = 0.2;
  view[1] = 3.2;

  // Should fixed dimension be generic, or should we just provide a couple of
  // special cases, in particular for Tof?
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
  DatasetIterator<Slab<Variable::Value>, Variable::Int> it(d, {Dimension::Tof});
  // it.get<double>(); // Does not compile, since we cannot get a single double.
  it.get<Variable::Int>();
}

TEST(DatasetIterator, single_column_edges) {
  Dataset d;
  auto edges = makeDataArray<Variable::Value>(Dimensions(Dimension::Tof, 3), 3);
  d.insertAsEdge(Dimension::Tof, edges);
  d.insert<Variable::Int>("name2", Dimensions(Dimension::Tof, 2), 2);
  auto &view = d.get<Variable::Value>();
  ASSERT_EQ(view.size(), 3);
  view[0] = 0.2;
  view[2] = 2.2;

  DatasetIterator<Variable::Value> it(d);
  ASSERT_EQ(it.get<Variable::Value>(), 0.2);
  it.increment();
  ASSERT_EQ(it.get<Variable::Value>(), 0.0);
  ASSERT_FALSE(it.atLast());
  it.increment();
  ASSERT_EQ(it.get<Variable::Value>(), 2.2);
  ASSERT_TRUE(it.atLast());
}

TEST(DatasetIterator, multi_column_edges) {
  Dataset d;
  auto edges = makeDataArray<Variable::Value>(Dimensions(Dimension::Tof, 3), 3);
  d.insertAsEdge(Dimension::Tof, edges);
  d.insert<Variable::Int>("name2", Dimensions(Dimension::Tof, 2), 2);
  auto &view = d.get<Variable::Value>();
  view[0] = 0.2;
  view[2] = 2.2;

  // Cannot simultaneously iterate edges and non-edges, so this throws.
  ASSERT_ANY_THROW((DatasetIterator<Variable::Value, Variable::Int>(d)));
  // TODO implement a way to iterate bins, maybe using a helper class
  // Bins<Variable::Tof> that can be used as a tag?
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

template <class Base, class T> struct GetterMixin;

template <class Base> struct GetterMixin<Base, double> {
  double getDouble() { return 1.5; }
};

template <class Base> struct GetterMixin<Base, int> {
  int getInt() { return 1; }
};

template <class... Ts> struct View : public GetterMixin<View<Ts...>, Ts>... {};

TEST(GetterMixins, compilation_test) {
  View<double, int> view;
  // The actual "test" is that the following code compiles:
  EXPECT_EQ(view.getDouble(), 1.5);
  EXPECT_EQ(view.getInt(), 1);
}

template <class T> constexpr int type_to_id();
template <> constexpr int type_to_id<double>() { return 0; }
template <> constexpr int type_to_id<int>() { return 1; }
template <> constexpr int type_to_id<char>() { return 2; }

template <int N> struct id_to_type;
template <> struct id_to_type<0> { using type = double; };
template <> struct id_to_type<1> { using type = int; };
template <> struct id_to_type<2> { using type = char; };
template <int N> using id_to_type_t = typename id_to_type<N>::type;

template <class Sorted, size_t... Is>
auto sort_types_impl(std::index_sequence<Is...>) {
  return std::tuple<
      id_to_type_t<boost::mpl::at_c<Sorted, Is>::type::value>...>{};
}

template <class... Ts> auto sort_types() {
  using Unsorted = boost::mpl::vector_c<int, type_to_id<Ts>()...>;
  return sort_types_impl<typename boost::mpl::sort<Unsorted>::type>(
      std::make_index_sequence<sizeof...(Ts)>{});
}

// Named "Set" because the order of types in the declaration does not matter,
// yields the same type.
template <class... Ts> using Set = decltype(sort_types<Ts...>());

TEST(SortTypes, same) {
  using unsorted1 = boost::mpl::vector_c<int, 4, 3, 1>;
  using unsorted2 = boost::mpl::vector_c<int, 4, 1, 3>;
  ASSERT_EQ(typeid(boost::mpl::sort<unsorted1>::type),
            typeid(boost::mpl::sort<unsorted2>::type));
}

TEST(SortTypes, different) {
  using unsorted1 = boost::mpl::vector_c<int, 4, 3, 1>;
  using unsorted2 = boost::mpl::vector_c<int, 4, 1, 2>;
  ASSERT_NE(typeid(boost::mpl::sort<unsorted1>::type),
            typeid(boost::mpl::sort<unsorted2>::type));
}

TEST(SortTypes, sort) {
  auto t = sort_types<char, double, int>();
  ASSERT_EQ(typeid(decltype(t)), typeid(std::tuple<double, int, char>));
}

TEST(SortTypes, type) {
  Set<char, double, int> a;
  Set<double, char, int> b;
  ASSERT_EQ(typeid(decltype(a)), typeid(decltype(b)));
}
