// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @author Simon Heybrock
#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "scipp-units_export.h"

namespace scipp::units {

class SCIPP_UNITS_EXPORT Dim {
public:
  enum class Id : uint16_t {
    Invalid,
    None,
    InternalStructureComponent,
    InternalStructureRow,
    InternalStructureColumn,
    InternalHistogram,
    InternalSort,
    InternalAccumulate,
    Energy,
    Event,
    Group,
    Position,
    Row,
    Temperature,
    Time,
    Wavelength,
    X,
    Y,
    Z
  };

  constexpr static auto Invalid = Id::Invalid;
  constexpr static auto None = Id::None;
  constexpr static auto InternalStructureComponent =
      Id::InternalStructureComponent;
  constexpr static auto InternalStructureRow = Id::InternalStructureRow;
  constexpr static auto InternalStructureColumn = Id::InternalStructureColumn;
  constexpr static auto InternalHistogram = Id::InternalHistogram;
  constexpr static auto InternalSort = Id::InternalSort;
  constexpr static auto InternalAccumulate = Id::InternalAccumulate;

  constexpr static auto Energy = Id::Energy;
  constexpr static auto Event = Id::Event;
  constexpr static auto Group = Id::Group;
  constexpr static auto Position = Id::Position;
  constexpr static auto Row = Id::Row;
  constexpr static auto Temperature = Id::Temperature;
  constexpr static auto Time = Id::Time;
  constexpr static auto Wavelength = Id::Wavelength;
  constexpr static auto X = Id::X;
  constexpr static auto Y = Id::Y;
  constexpr static auto Z = Id::Z;

  constexpr Dim() : m_id(Id::Invalid) {}
  // cppcheck-suppress noExplicitConstructor
  constexpr Dim(const Id id) : m_id(id) {}
  explicit Dim(const std::string &label);

  constexpr Id id() const noexcept { return m_id; }

  std::string name() const;

  constexpr bool operator==(const Dim &other) const noexcept {
    return m_id == other.m_id;
  }
  constexpr bool operator!=(const Dim &other) const noexcept {
    return m_id != other.m_id;
  }
  constexpr bool operator<(const Dim &other) const noexcept {
    return m_id < other.m_id;
  }

private:
  Id m_id;
};

SCIPP_UNITS_EXPORT std::string to_string(const Dim dim);

} // namespace scipp::units

// Hashing required temporarily while we use Dim as a key for the coord dict.
namespace std {
template <> struct hash<scipp::units::Dim> {
  std::size_t operator()(const scipp::units::Dim &k) const {
    return hash<scipp::units::Dim::Id>()(k.id());
  }
};
} // namespace std

namespace scipp {
using scipp::units::Dim;
}
