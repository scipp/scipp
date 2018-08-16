#ifndef TAGS_H
#define TAGS_H

#include <tuple>
#include <vector>

#include <gsl/gsl_util>

namespace detail {
template <class T, class Tuple> struct index;
template <class T, class... Types> struct index<T, std::tuple<T, Types...>> {
  static const std::size_t value = 0;
};
template <class T, class U, class... Types>
struct index<T, std::tuple<U, Types...>> {
  static const std::size_t value = 1 + index<T, std::tuple<Types...>>::value;
};
struct ReturnByValuePolicy {};
}

struct Coord {
  struct X {
    using type = double;
  };
  struct Y {
    using type = double;
  };
  struct Z {
    using type = double;
  };
  struct Tof {
    using type = double;
  };
  struct SpectrumNumber {
    using type = int32_t;
  };
  struct DetectorPosition {
    // Dummy for now, should be something like Eigen::Vector3d.
    using type = double;
  };
  struct DetectorGrouping {
    // Dummy for now, or sufficient like this?
    using type = std::vector<gsl::index>;
  };
  struct SpectrumPosition : public detail::ReturnByValuePolicy {
    // TODO This is a virtual/derived tag, do we need to specify type?
    using type = double;
  };

  using tags = std::tuple<X, Y, Z, Tof, SpectrumNumber, DetectorPosition,
                          DetectorGrouping, SpectrumPosition>;
};

class Histogram;
struct Data {
  struct Tof {
    using type = double;
  };
  struct Value {
    using type = double;
  };
  struct Error {
    using type = double;
  };
  struct Int {
    using type = int64_t;
  };
  struct DimensionSize {
    using type = gsl::index;
  };
  struct String {
    using type = std::string;
  };
  struct Histogram {
    using type = ::Histogram;
  };

  using tags =
      std::tuple<Tof, Value, Error, Int, DimensionSize, String, Histogram>;
};

template <class T>
static constexpr uint16_t tag_id =
    detail::index<std::remove_const_t<T>,
                  decltype(std::tuple_cat(std::declval<Coord::tags>(),
                                          std::declval<Data::tags>()))>::value;
template <class T>
static constexpr bool is_coord =
    tag_id<T> < std::tuple_size<Coord::tags>::value;

class DataBin {
public:
  DataBin(const double left, const double right)
      : m_left(left), m_right(right) {}

  double center() const { return 0.5 * (m_left + m_right); }
  double width() const { return m_right - m_left; }
  double left() const { return m_left; }
  double right() const { return m_right; }

private:
  double m_left;
  double m_right;
};

template <class T> struct Bin { using type = DataBin; };

template <class Tag> struct element_return_type {
  using type = std::conditional_t<
      std::is_base_of<detail::ReturnByValuePolicy, Tag>::value,
      typename Tag::type,
      std::conditional_t<std::is_const<Tag>::value, const typename Tag::type &,
                         typename Tag::type &>>;
};

template <class Tag>
using element_return_type_t = typename element_return_type<Tag>::type;

#endif // TAGS_H
