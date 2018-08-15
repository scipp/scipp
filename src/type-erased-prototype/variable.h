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
  struct SpectrumPosition {
    // TODO This is a virtual/derived tag, do we need to specify type?
    using type = double;
  };

  using tags = std::tuple<Tof, SpectrumNumber, DetectorPosition,
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
  struct Histogram {
    using type = ::Histogram;
  };

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

template <class T> struct Bin { using value_type = T; };

template <class Tag> struct element_reference_type;

template <> struct element_reference_type<Coord::Tof> {
  using type = double &;
};
template <> struct element_reference_type<const Coord::Tof> {
  using type = const double &;
};

template <> struct element_reference_type<Coord::DetectorPosition> {
  using type = double &;
};
template <> struct element_reference_type<const Coord::DetectorPosition> {
  using type = const double &;
};

template <> struct element_reference_type<Coord::SpectrumPosition> {
  // Note: No reference.
  using type = double;
};
template <> struct element_reference_type<const Coord::SpectrumPosition> {
  // Note: No reference.
  using type = const double;
};

template <> struct element_reference_type<Data::Tof> { using type = double &; };
template <> struct element_reference_type<const Data::Tof> {
  using type = const double &;
};

template <> struct element_reference_type<Bin<Data::Tof>> {
  // Note: No reference.
  using type = DataBin;
};
template <> struct element_reference_type<Bin<const Data::Tof>> {
  // Note: No reference.
  using type = DataBin;
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

template <class Tag>
using element_reference_type_t = typename element_reference_type<Tag>::type;

#endif // VARIABLE_H
