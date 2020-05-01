// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @author Simon Heybrock
#pragma once

#include <functional>
#include <mutex>
#include <unordered_map>

namespace scipp::units {

class Dim {
public:
  enum class Id : uint16_t {
    Detector,
    DSpacing,
    Energy,
    EnergyTransfer,
    Group,
    Position,
    PulseTime,
    Q,
    QSquared,
    Qx,
    Qy,
    Qz,
    Row,
    ScatteringAngle,
    Spectrum,
    Temperature,
    Time,
    Tof,
    Wavelength,
    X,
    Y,
    Z,
    Invalid
  };

  constexpr static auto Detector = Id::Detector;
  constexpr static auto DSpacing = Id::DSpacing;
  constexpr static auto Energy = Id::Energy;
  constexpr static auto EnergyTransfer = Id::EnergyTransfer;
  constexpr static auto Group = Id::Group;
  constexpr static auto Invalid = Id::Invalid;
  constexpr static auto Position = Id::Position;
  constexpr static auto PulseTime = Id::PulseTime;
  constexpr static auto Q = Id::Q;
  constexpr static auto Qx = Id::Qx;
  constexpr static auto Qy = Id::Qy;
  constexpr static auto Qz = Id::Qz;
  constexpr static auto QSquared = Id::QSquared;
  constexpr static auto Row = Id::Row;
  constexpr static auto ScatteringAngle = Id::ScatteringAngle;
  constexpr static auto Spectrum = Id::Spectrum;
  constexpr static auto Temperature = Id::Temperature;
  constexpr static auto Time = Id::Time;
  constexpr static auto Tof = Id::Tof;
  constexpr static auto Wavelength = Id::Wavelength;
  constexpr static auto X = Id::X;
  constexpr static auto Y = Id::Y;
  constexpr static auto Z = Id::Z;

  constexpr Dim() : m_id(Id::Invalid) {}
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
  static std::unordered_map<std::string, Id> builtin_ids;
  static std::unordered_map<std::string, Id> custom_ids;
  static std::mutex mutex;
};

std::string to_string(const Dim dim);

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
