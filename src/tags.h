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

#include <boost/container/small_vector.hpp>
#include <gsl/gsl_util>

#include "dimension.h"
#include "traits.h"
#include "unit.h"
#include "value_with_delta.h"

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
} // namespace detail

class Dataset;

namespace detail {
struct CoordDef {
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
  struct MonitorTof {
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
  struct DetectorIsMonitor {
    using type = char;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorMask {
    using type = char;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorRotation {
    // Dummy for now, should be something like Eigen::Quaterniond.
    using type = std::array<double, 4>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorPosition {
    // Dummy for now, should be something like Eigen::Vector3d.
    using type = double;
    static constexpr auto unit = Unit::Id::Length;
  };
  struct DetectorGrouping {
    // Dummy for now, or sufficient like this?
    using type = boost::container::small_vector<gsl::index, 1>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct SpectrumPosition : public detail::ReturnByValuePolicy {
    using type = double;
    static constexpr auto unit = Unit::Id::Length;
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
  struct ComponentRotation {
    using type = std::array<double, 4>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct ComponentPosition {
    using type = std::array<double, 3>;
    static constexpr auto unit = Unit::Id::Length;
  };
  struct ComponentParent {
    using type = gsl::index;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct ComponentChildren {
    using type = std::vector<gsl::index>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct ComponentScale {
    using type = std::array<double, 3>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct ComponentShape {
    using type = std::shared_ptr<std::array<double, 100>>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct ComponentName {
    using type = std::string;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct ComponentSubtree {
    using type = std::vector<gsl::index>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorSubtree {
    using type = std::vector<gsl::index>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct ComponentSubtreeRange {
    using type = std::pair<gsl::index, gsl::index>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorSubtreeRange {
    using type = std::pair<gsl::index, gsl::index>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorParent {
    using type = gsl::index;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorScale {
    using type = std::array<double, 3>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct DetectorShape {
    using type = std::shared_ptr<std::array<double, 100>>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };
  struct FuzzyTemperature {
    using type = ValueWithDelta<double>;
    static constexpr auto unit = Unit::Id::Dimensionless;
  };

  using tags = std::tuple<
      X, Y, Z, Tof, MonitorTof, DetectorId, SpectrumNumber, DetectorIsMonitor,
      DetectorMask, DetectorRotation, DetectorPosition, DetectorGrouping,
      SpectrumPosition, RowLabel, Polarization, Temperature, FuzzyTemperature,
      Time, TimeInterval, Mask, ComponentRotation, ComponentPosition,
      ComponentParent, ComponentChildren, ComponentScale, ComponentShape,
      ComponentName, ComponentSubtree, DetectorSubtree, ComponentSubtreeRange,
      DetectorSubtreeRange, DetectorParent, DetectorScale, DetectorShape>;
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
  struct History {
    using type = std::vector<std::string>;
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
                          DimensionSize, String, History, Events, Table>;
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

template <class TagDefinition>
struct TagImpl : public Tag, TagDefinition {
  constexpr TagImpl() : Tag(tag_id<TagDefinition>) {}
};

} // namespace detail

struct Coord {
  using X = detail::TagImpl<detail::CoordDef::X>;
  using Y = detail::TagImpl<detail::CoordDef::Y>;
  using Z = detail::TagImpl<detail::CoordDef::Z>;
  using Tof = detail::TagImpl<detail::CoordDef::Tof>;
  using MonitorTof = detail::TagImpl<detail::CoordDef::MonitorTof>;
  using DetectorId = detail::TagImpl<detail::CoordDef::DetectorId>;
  using SpectrumNumber = detail::TagImpl<detail::CoordDef::SpectrumNumber>;
  using DetectorIsMonitor =
      detail::TagImpl<detail::CoordDef::DetectorIsMonitor>;
  using DetectorMask = detail::TagImpl<detail::CoordDef::DetectorMask>;
  using DetectorRotation = detail::TagImpl<detail::CoordDef::DetectorRotation>;
  using DetectorPosition = detail::TagImpl<detail::CoordDef::DetectorPosition>;
  using DetectorGrouping = detail::TagImpl<detail::CoordDef::DetectorGrouping>;
  using SpectrumPosition = detail::TagImpl<detail::CoordDef::SpectrumPosition>;
  using RowLabel = detail::TagImpl<detail::CoordDef::RowLabel>;
  using Polarization = detail::TagImpl<detail::CoordDef::Polarization>;
  using Temperature = detail::TagImpl<detail::CoordDef::Temperature>;
  using FuzzyTemperature = detail::TagImpl<detail::CoordDef::FuzzyTemperature>;
  using Time = detail::TagImpl<detail::CoordDef::Time>;
  using TimeInterval = detail::TagImpl<detail::CoordDef::TimeInterval>;
  using Mask = detail::TagImpl<detail::CoordDef::Mask>;
  using ComponentRotation =
      detail::TagImpl<detail::CoordDef::ComponentRotation>;
  using ComponentPosition =
      detail::TagImpl<detail::CoordDef::ComponentPosition>;
  using ComponentParent = detail::TagImpl<detail::CoordDef::ComponentParent>;
  using ComponentChildren =
      detail::TagImpl<detail::CoordDef::ComponentChildren>;
  using ComponentScale = detail::TagImpl<detail::CoordDef::ComponentScale>;
  using ComponentShape = detail::TagImpl<detail::CoordDef::ComponentShape>;
  using ComponentName = detail::TagImpl<detail::CoordDef::ComponentName>;
  using ComponentSubtree = detail::TagImpl<detail::CoordDef::ComponentSubtree>;
  using DetectorSubtree = detail::TagImpl<detail::CoordDef::DetectorSubtree>;
  using ComponentSubtreeRange =
      detail::TagImpl<detail::CoordDef::ComponentSubtreeRange>;
  using DetectorSubtreeRange =
      detail::TagImpl<detail::CoordDef::DetectorSubtreeRange>;
  using DetectorParent = detail::TagImpl<detail::CoordDef::DetectorParent>;
  using DetectorScale = detail::TagImpl<detail::CoordDef::DetectorScale>;
  using DetectorShape = detail::TagImpl<detail::CoordDef::DetectorShape>;

  using tags = std::tuple<
      X, Y, Z, Tof, MonitorTof, DetectorId, SpectrumNumber, DetectorIsMonitor,
      DetectorMask, DetectorRotation, DetectorPosition, DetectorGrouping,
      SpectrumPosition, RowLabel, Polarization, Temperature, FuzzyTemperature,
      Time, TimeInterval, Mask, ComponentRotation, ComponentPosition,
      ComponentParent, ComponentChildren, ComponentScale, ComponentShape,
      ComponentName, ComponentSubtree, DetectorSubtree, ComponentSubtreeRange,
      DetectorSubtreeRange, DetectorParent, DetectorScale, DetectorShape>;
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
  using History = detail::TagImpl<detail::DataDef::History>;
  using Events = detail::TagImpl<detail::DataDef::Events>;
  using Table = detail::TagImpl<detail::DataDef::Table>;

  using tags = std::tuple<Tof, PulseTime, Value, Variance, StdDev, Int,
                          DimensionSize, String, History, Events, Table>;
};

struct Attr {
  using ExperimentLog = detail::TagImpl<detail::AttrDef::ExperimentLog>;

  using tags = std::tuple<ExperimentLog>;
};

using Tags = decltype(std::tuple_cat(std::declval<Coord::tags>(),
                                     std::declval<Data::tags>(),
                                     std::declval<Attr::tags>()));

template <class T>
static constexpr bool is_coord = T{} < std::tuple_size<Coord::tags>::value;
template <class T>
static constexpr bool is_attr = T{} >= std::tuple_size<Coord::tags>::value +
                                           std::tuple_size<Data::tags>::value;
template <class T> static constexpr bool is_data = !is_coord<T> && !is_attr<T>;

template <class Tag> constexpr bool is_dimension_coordinate = false;
template <> constexpr bool is_dimension_coordinate<Coord::Tof> = true;
template <> constexpr bool is_dimension_coordinate<Coord::X> = true;
template <> constexpr bool is_dimension_coordinate<Coord::Y> = true;
template <> constexpr bool is_dimension_coordinate<Coord::Z> = true;
template <>
constexpr bool is_dimension_coordinate<Coord::SpectrumNumber> = true;
template <> constexpr bool is_dimension_coordinate<Coord::RowLabel> = true;

template <class Tag> constexpr Dimension coordinate_dimension = Dim::Invalid;
template <> constexpr Dimension coordinate_dimension<Coord::Tof> = Dim::Tof;
template <> constexpr Dimension coordinate_dimension<Coord::X> = Dim::X;
template <> constexpr Dimension coordinate_dimension<Coord::Y> = Dim::Y;
template <> constexpr Dimension coordinate_dimension<Coord::Z> = Dim::Z;
template <>
constexpr Dimension coordinate_dimension<Coord::SpectrumNumber> = Dim::Spectrum;
template <>
constexpr Dimension coordinate_dimension<Coord::RowLabel> = Dim::Row;

template <class... Ts>
constexpr std::array<bool, std::tuple_size<Tags>::value>
make_is_dimension_coordinate(const std::tuple<Ts...> &) {
  return {is_dimension_coordinate<Ts>...};
}

template <class... Ts>
constexpr std::array<Dim, std::tuple_size<Tags>::value>
make_coordinate_dimension(const std::tuple<Ts...> &) {
  return {coordinate_dimension<Ts>...};
}

constexpr auto isDimensionCoord = make_is_dimension_coordinate(Tags{});
constexpr auto coordDimension = make_coordinate_dimension(Tags{});

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

template <class Tags> struct element_return_type<Bin<Tags>> {
  using type = DataBin;
};

template <class... Ts> class DatasetViewImpl;
template <class... Tags> struct element_return_type<DatasetViewImpl<Tags...>> {
  using type = DatasetViewImpl<Tags...>;
};

template <class Tag>
using element_return_type_t = typename element_return_type<Tag>::type;

#endif // TAGS_H
