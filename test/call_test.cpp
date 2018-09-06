/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <tuple>
#include <utility>

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
  static void apply(double &x, const int &i) { x *= i; }
};

struct IntAlgorithm {
  static void apply(int &x) { ++x; }
};

// For basic types such as double, int, ... there can typically be multiple
// matching columns. How should we specific which ones to apply to?
// - Specify column IDs at call site?

// We want to be able to write "normal" code that uses overloads in case
// multiple cases need to be supported. How can we handle this?
// - List of all possible column type combinations (up to certain argument
// count) built at compile time -> overload resolution?

TEST(CallWrappers, call_DatasetAlgorithm) {
  Dataset d;
  d = call<DatasetAlgorithm>(std::move(d));
}

TEST(CallWrappers, call_SingleItemAlgorithm) {
  Dataset d;
  d.add<Data::Value>("name1");
  d = call<SingleItemAlgorithm>(std::move(d));
  ASSERT_EQ(d.get<Data::Value>()[0], 1.5);
}

TEST(CallWrappers, call_TwoItemAlgorithm) {
  Dataset d;
  d.add<Data::Value>("name1");
  d.add<Data::Int>("name2");
  // TODO should not compile if const is removed from int argument.
  // d.addDimension(Dimension::Tof, 10);
  // d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  d.get<Data::Int>()[0] = 2;
  d = call<SingleItemAlgorithm>(std::move(d));
  d = call<TwoItemAlgorithm>(std::move(d));
  ASSERT_EQ(d.get<Data::Value>()[0], 3.0);
}

// Begin example of finding overloads at compile time.
struct AlgWithOverloads {
  static std::string f(double x) { return "double"; }
  static std::string f(int x) { return "int"; }
};

constexpr std::tuple<double, int, char> allowed_types;

BOOST_TTI_HAS_FUNCTION(f);
template <class Alg, class Enable, class Arg> struct supported {
  using type = std::tuple<>;
};
template <class Alg, class Arg>
struct supported<Alg,
                 std::enable_if_t<has_function_f<
                     const Alg, std::string, boost::mpl::vector<Arg>>::value>,
                 Arg> {
  using type = std::tuple<Arg>;
};

template <class Alg, class Types>
void try_get_column_impl(Dataset &d, Types types) {
  // no more types in tuple.
}

template <class Alg, class Types, size_t I, size_t... Is>
void try_get_column_impl(Dataset &d, Types types) {
  using ColType =
      std::vector<std::remove_const_t<std::tuple_element_t<I, Types>>>;
  try {
    auto &col = d.get<ColType>();
    fprintf(stderr, "%s\n", Alg::f(col[0]).c_str());
  } catch (const std::runtime_error &) {
    fprintf(stderr, "Column for %s not found, skipping.\n",
            typeid(ColType).name());
  }
  try_get_column_impl<Alg, Types, Is...>(d, types);
}

template <class Alg, class Types, size_t... Is>
void try_get_column(Dataset &d, Types types, std::index_sequence<Is...>) {
  try_get_column_impl<Alg, Types, Is...>(d, types);
}

template <class Alg, size_t... Is>
gsl::index run_impl(Dataset &d, std::index_sequence<Is...>) {
  auto types = std::tuple_cat(typename supported<
      Alg, void, std::tuple_element_t<Is, decltype(allowed_types)>>::type{}...);
  try_get_column<Alg>(
      d, types,
      std::make_index_sequence<std::tuple_size<decltype(types)>::value>{});
  // convert allowed_types into smaller tuple of supported_types
  return std::tuple_size<decltype(types)>::value;
}

template <class Alg> gsl::index run(Dataset &d) {
  return run_impl<Alg>(d,
                       std::make_index_sequence<
                           std::tuple_size<decltype(allowed_types)>::value>{});
}

TEST(FindOverloads, type_erased) {
  Dataset d;
  d.add<Data::Value>("name1");
  // d.addColumn<int>("name2");
  ASSERT_EQ(run<AlgWithOverloads>(d), 2);
}
// End example of finding overloads at compile time.
