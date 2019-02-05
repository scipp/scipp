/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef TAGS_H
#define TAGS_H

#include <memory>
#include <tuple>
#include <vector>

#include <Eigen/Dense>
#include <boost/container/small_vector.hpp>
#include <gsl/gsl_util>

#include "dimension.h"
#include "traits.h"
#include "unit.h"
#include "value_with_delta.h"

// Adding new tags
// ===============
//
// 1. Add the tag class to either CoordDef, DataDef, or AttrDef in namespace
//    detail, and list the class in the corresponding`tags` tuple.
// 2. Add a `using` clause for the tag in Coord, Data, or Attr.
//
// Optionally:
// 3. If the underlying type is new, explicit instantiations in variable.cpp may
//    need to be added.
// 4. If the new tag corresponds to a dimension, `is_dimension_coordinate` and
//    `coordinate_dimension` need to be set correctly.
// 5. For support in Python, the tag needs to be exported in
//    `exports/dataset.cpp` and a fair number of methods in the same file need
//    to be modified to support the new tag.

class Tag {
public:
  constexpr Tag() : m_value(0) {}
  constexpr explicit Tag(uint16_t value) : m_value(value) {}
  constexpr uint16_t value() const { return m_value; }

  constexpr bool operator==(const Tag other) const {
    return m_value == other.m_value;
  }
  constexpr bool operator!=(const Tag other) const {
    return m_value != other.m_value;
  }

  constexpr bool operator<(const int32_t value) const {
    return m_value < value;
  }
  constexpr bool operator>=(const int32_t value) const {
    return m_value >= value;
  }

private:
  uint16_t m_value;
};

namespace detail {
struct ReturnByValuePolicy {};
// Returns by value for const access (allowing for deriving from other
// variables), but by reference if access is non-const (other code must ensure
// that no references to temporaries are returned). The main example for this is
// a spectrum position: It can be derived from detector positions (averaged
// using Coord::DetectorGrouping) or be provided directly. In the latter case
// the positions may be modified.
struct ReturnByValueIfConstPolicy {};
} // namespace detail

class Dataset;

namespace detail {
struct CoordDef {
  struct Monitor {
    using type = Dataset;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  // TODO Should we name this `Detectors` and `Components` instead, or find some
  // more generic terms?
  struct DetectorInfo {
    using type = Dataset;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct ComponentInfo {
    using type = Dataset;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct X {
    using type = double;
    static constexpr auto unit = Unit::Id::Length;
  };
  struct Y {
    using type = double;
    static constexpr auto unit = Unit::Id::Length;
  };
  struct Z {
    using type = double;
    static constexpr auto unit = Unit::Id::Length;
  };
  struct Tof {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Energy {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DeltaE {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Ei {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Ef {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorId {
    using type = int32_t;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct SpectrumNumber {
    using type = int32_t;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorGrouping {
    // Dummy for now, or sufficient like this?
    using type = boost::container::small_vector<gsl::index, 1>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct RowLabel {
    using type = std::string;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Polarization {
    // Dummy for now
    using type = std::string;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Temperature {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Time {
    using type = int64_t;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct TimeInterval {
    using type = std::pair<int64_t, int64_t>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Mask {
    using type = char;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct FuzzyTemperature {
    using type = ValueWithDelta<double>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Position : public detail::ReturnByValueIfConstPolicy {
    using type = Eigen::Vector3d;
    static constexpr auto unit = Unit::Id::Length;
  };

  using tags = std::tuple<Monitor, DetectorInfo, ComponentInfo, X, Y, Z, Tof,
                          Energy, DeltaE, Ei, Ef, DetectorId, SpectrumNumber,
                          DetectorGrouping, RowLabel, Polarization, Temperature,
                          FuzzyTemperature, Time, TimeInterval, Mask, Position>;
};

struct DataDef {
  struct Tof {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct PulseTime {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Value {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Variance {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct StdDev : public detail::ReturnByValuePolicy {
    using type = double;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Int {
    using type = int64_t;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DimensionSize {
    using type = gsl::index;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct String {
    using type = std::string;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Events {
    using type = Dataset;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct Table {
    using type = Dataset;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };

  using tags = std::tuple<Tof, PulseTime, Value, Variance, StdDev, Int,
                          DimensionSize, String, Events, Table>;
};

struct AttrDef {
  struct ExperimentLog {
    using type = Dataset;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };

  using tags = std::tuple<ExperimentLog>;
};

using Tags = decltype(std::tuple_cat(std::declval<detail::CoordDef::tags>(),
                                     std::declval<detail::DataDef::tags>(),
                                     std::declval<detail::AttrDef::tags>()));
template <class T>
static constexpr uint16_t tag_id =
    detail::index<std::remove_const_t<T>, Tags>::value;

template <class TagDefinition> struct TagImpl : public Tag, TagDefinition {
  constexpr TagImpl() : Tag(tag_id<TagDefinition>) {}
};

} // namespace detail

struct Coord {
  using Monitor = detail::TagImpl<detail::CoordDef::Monitor>;
  using DetectorInfo = detail::TagImpl<detail::CoordDef::DetectorInfo>;
  using ComponentInfo = detail::TagImpl<detail::CoordDef::ComponentInfo>;
  using X = detail::TagImpl<detail::CoordDef::X>;
  using Y = detail::TagImpl<detail::CoordDef::Y>;
  using Z = detail::TagImpl<detail::CoordDef::Z>;
  using Tof = detail::TagImpl<detail::CoordDef::Tof>;
  using Energy = detail::TagImpl<detail::CoordDef::Energy>;
  using DeltaE = detail::TagImpl<detail::CoordDef::DeltaE>;
  using Ei = detail::TagImpl<detail::CoordDef::Ei>;
  using Ef = detail::TagImpl<detail::CoordDef::Ef>;
  using DetectorId = detail::TagImpl<detail::CoordDef::DetectorId>;
  using SpectrumNumber = detail::TagImpl<detail::CoordDef::SpectrumNumber>;
  using DetectorGrouping = detail::TagImpl<detail::CoordDef::DetectorGrouping>;
  using RowLabel = detail::TagImpl<detail::CoordDef::RowLabel>;
  using Polarization = detail::TagImpl<detail::CoordDef::Polarization>;
  using Temperature = detail::TagImpl<detail::CoordDef::Temperature>;
  using FuzzyTemperature = detail::TagImpl<detail::CoordDef::FuzzyTemperature>;
  using Time = detail::TagImpl<detail::CoordDef::Time>;
  using TimeInterval = detail::TagImpl<detail::CoordDef::TimeInterval>;
  using Mask = detail::TagImpl<detail::CoordDef::Mask>;
  using Position = detail::TagImpl<detail::CoordDef::Position>;
};

struct Data {
  using Tof = detail::TagImpl<detail::DataDef::Tof>;
  using PulseTime = detail::TagImpl<detail::DataDef::PulseTime>;
  using Value = detail::TagImpl<detail::DataDef::Value>;
  using Variance = detail::TagImpl<detail::DataDef::Variance>;
  using StdDev = detail::TagImpl<detail::DataDef::StdDev>;
  using Int = detail::TagImpl<detail::DataDef::Int>;
  using DimensionSize = detail::TagImpl<detail::DataDef::DimensionSize>;
  using String = detail::TagImpl<detail::DataDef::String>;
  using Events = detail::TagImpl<detail::DataDef::Events>;
  using Table = detail::TagImpl<detail::DataDef::Table>;
};

struct Attr {
  using ExperimentLog = detail::TagImpl<detail::AttrDef::ExperimentLog>;
};

template <class T>
static constexpr bool is_coord =
    T{} < std::tuple_size<detail::CoordDef::tags>::value;
template <class T>
static constexpr bool is_attr =
    T{} >= std::tuple_size<detail::CoordDef::tags>::value +
               std::tuple_size<detail::DataDef::tags>::value;
template <class T> static constexpr bool is_data = !is_coord<T> && !is_attr<T>;

namespace detail {
template <class Tag> constexpr bool is_dimension_coordinate = false;
template <> constexpr bool is_dimension_coordinate<CoordDef::Tof> = true;
template <> constexpr bool is_dimension_coordinate<CoordDef::Energy> = true;
template <> constexpr bool is_dimension_coordinate<CoordDef::DeltaE> = true;
template <> constexpr bool is_dimension_coordinate<CoordDef::X> = true;
template <> constexpr bool is_dimension_coordinate<CoordDef::Y> = true;
template <> constexpr bool is_dimension_coordinate<CoordDef::Z> = true;
template <>
constexpr bool is_dimension_coordinate<CoordDef::SpectrumNumber> = true;
template <> constexpr bool is_dimension_coordinate<CoordDef::RowLabel> = true;

template <class Tag> constexpr Dim coordinate_dimension = Dim::Invalid;
template <> constexpr Dim coordinate_dimension<CoordDef::Tof> = Dim::Tof;
template <> constexpr Dim coordinate_dimension<CoordDef::Energy> = Dim::Energy;
template <> constexpr Dim coordinate_dimension<CoordDef::DeltaE> = Dim::DeltaE;
template <> constexpr Dim coordinate_dimension<CoordDef::X> = Dim::X;
template <> constexpr Dim coordinate_dimension<CoordDef::Y> = Dim::Y;
template <> constexpr Dim coordinate_dimension<CoordDef::Z> = Dim::Z;
template <>
constexpr Dim coordinate_dimension<CoordDef::SpectrumNumber> = Dim::Spectrum;
template <> constexpr Dim coordinate_dimension<CoordDef::RowLabel> = Dim::Row;
} // namespace detail

template <class... Ts>
constexpr std::array<bool, std::tuple_size<detail::Tags>::value>
make_is_dimension_coordinate(const std::tuple<Ts...> &) {
  return {detail::is_dimension_coordinate<Ts>...};
}

template <class... Ts>
constexpr std::array<Dim, std::tuple_size<detail::Tags>::value>
make_coordinate_dimension(const std::tuple<Ts...> &) {
  return {detail::coordinate_dimension<Ts>...};
}

// Coordinates linked to a specific dimension, e.g., Coord::X to Dim::X. See
// also the xarray documentation regarding its distinction getween "dimension
// coordinates" and "non-dimension coordinates".
constexpr auto isDimensionCoord = make_is_dimension_coordinate(detail::Tags{});
constexpr auto coordDimension = make_coordinate_dimension(detail::Tags{});

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

template <class D, class Tag> struct element_return_type {
  using type = std::conditional_t<
      std::is_base_of<detail::ReturnByValuePolicy, Tag>::value,
      typename Tag::type,
      std::conditional_t<
          std::is_const<D>::value || std::is_const<Tag>::value,
          std::conditional_t<
              std::is_base_of<detail::ReturnByValueIfConstPolicy, Tag>::value,
              typename Tag::type, const typename Tag::type &>,
          typename Tag::type &>>;
};

template <class D, class Tags> struct element_return_type<D, Bin<Tags>> {
  using type = DataBin;
};

template <class D, class... Ts> class MDZipViewImpl;
template <class D, class... Tags>
struct element_return_type<D, MDZipViewImpl<D, Tags...>> {
  using type = MDZipViewImpl<D, Tags...>;
};

template <class D, class Tag>
using element_return_type_t = typename element_return_type<D, Tag>::type;

enum class DType { Unknown, Double, Float, Int32, Int64, String, Char };
template <class T> constexpr DType dtype = DType::Unknown;
template <> constexpr DType dtype<double> = DType::Double;
template <> constexpr DType dtype<float> = DType::Float;
template <> constexpr DType dtype<int32_t> = DType::Int32;
template <> constexpr DType dtype<int64_t> = DType::Int64;
template <> constexpr DType dtype<std::string> = DType::String;
template <> constexpr DType dtype<char> = DType::Char;

#endif // TAGS_H
