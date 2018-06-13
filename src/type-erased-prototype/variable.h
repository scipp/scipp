#ifndef VARIABLE_H
#define VARIABLE_H

#include <tuple>
#include <vector>

#include "index.h"

namespace detail {
template <class T, class Tuple> struct index;
template <class T, class... Types> struct index<T, std::tuple<T, Types...>> {
  static const std::size_t value = 0;
};
template <class T, class U, class... Types>
struct index<T, std::tuple<U, Types...>> {
  static const std::size_t value = 1 + index<T, std::tuple<Types...>>::value;
};
}

struct Coord {
  struct Tof {};
  struct SpectrumNumber {};

  using tags = std::tuple<Tof, SpectrumNumber>;
};

struct Data {
  struct Tof {};
  struct Value {};
  struct Error {};
  struct Int {};
  struct DimensionSize {};
  struct Histogram {};

  using tags = std::tuple<Tof, Value, Error, Int, DimensionSize, Histogram>;
};

template <class T>
static constexpr uint16_t tag_id =
    detail::index<std::remove_const_t<T>,
                  decltype(std::tuple_cat(std::declval<Coord::tags>(),
                                          std::declval<Data::tags>()))>::value;
template <class T>
static constexpr bool is_coord =
    tag_id<T> < std::tuple_size<Coord::tags>::value;

class Bin {
public:
  Bin(const double left, const double right) : m_left(left), m_right(right) {}

  double center() const { return 0.5 * (m_left + m_right); }
  double width() const { return m_right - m_left; }
  double left() const { return m_left; }
  double right() const { return m_right; }

private:
  double m_left;
  double m_right;
};

template <class T> struct Bins { using value_type = T; };

template <class Tag> struct variable_type;
template <class Tag> struct element_reference_type;

template <> struct variable_type<Coord::Tof> {
  using type = std::vector<double>;
};
template <> struct variable_type<Coord::SpectrumNumber> {
  using type = std::vector<int32_t>;
};

template <> struct variable_type<Data::Tof> {
  using type = std::vector<double>;
};
template <> struct variable_type<const Data::Tof> {
  using type = const std::vector<double>;
};

template <> struct variable_type<Bins<Data::Tof>> {
  using type = std::vector<double>;
};

template <> struct variable_type<Data::Value> {
  using type = std::vector<double>;
};
template <> struct variable_type<const Data::Value> {
  using type = const std::vector<double>;
};

template <> struct variable_type<Data::Error> {
  using type = std::vector<double>;
};
template <> struct variable_type<const Data::Error> {
  using type = const std::vector<double>;
};

template <> struct variable_type<Data::Int> {
  using type = std::vector<int64_t>;
};
template <> struct variable_type<const Data::Int> {
  using type = const std::vector<int64_t>;
};

template <> struct variable_type<Data::DimensionSize> {
  using type = std::vector<gsl::index>;
};
template <> struct variable_type<const Data::DimensionSize> {
  using type = const std::vector<gsl::index>;
};

class Histogram;
template <> struct variable_type<Data::Histogram> {
  using type = std::vector<Histogram>;
};
template <> struct variable_type<const Data::Histogram> {
  using type = const std::vector<Histogram>;
};

template <> struct element_reference_type<Data::Tof> { using type = double &; };
template <> struct element_reference_type<const Data::Tof> {
  using type = const double &;
};

template <> struct element_reference_type<Bins<Data::Tof>> {
  // Note: No reference.
  using type = Bin;
};
template <> struct element_reference_type<Bins<const Data::Tof>> {
  // Note: No reference.
  using type = Bin;
};

template <> struct element_reference_type<Data::Value> {
  using type = double &;
};
template <> struct element_reference_type<const Data::Value> {
  using type = const double &;
};

template <> struct element_reference_type<Data::Error> {
  using type = double &;
};
template <> struct element_reference_type<const Data::Error> {
  using type = const double &;
};

template <> struct element_reference_type<Data::Int> {
  using type = int64_t &;
};
template <> struct element_reference_type<const Data::Int> {
  using type = const int64_t &;
};

template <> struct element_reference_type<Data::DimensionSize> {
  using type = gsl::index &;
};
template <> struct element_reference_type<const Data::DimensionSize> {
  using type = const gsl::index &;
};

template <> struct element_reference_type<Data::Histogram> {
  using type = Histogram &;
};
template <> struct element_reference_type<const Data::Histogram> {
  using type = const Histogram &;
};

template <class Tag> using variable_type_t = typename variable_type<Tag>::type;
template <class Tag>
using element_reference_type_t = typename element_reference_type<Tag>::type;

#endif // VARIABLE_H
