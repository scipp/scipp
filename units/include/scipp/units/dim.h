// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @author Simon Heybrock
#ifndef SCIPP_UNITS_DIM_H
#define SCIPP_UNITS_DIM_H

#include <functional>
#include <unordered_map>

#include "scipp/units/dummy.h"
#include "scipp/units/neutron.h"

namespace scipp::units {
class Dim {
public:
  constexpr static auto Detector = DimId::Detector;
  constexpr static auto DSpacing = DimId::DSpacing;
  constexpr static auto Energy = DimId::Energy;
  constexpr static auto EnergyTransfer = DimId::EnergyTransfer;
  constexpr static auto Group = DimId::Row;
  constexpr static auto Invalid = DimId::Invalid;
  constexpr static auto Position = DimId::Position;
  constexpr static auto Q = DimId::Q;
  constexpr static auto Qx = DimId::Qx;
  constexpr static auto Qy = DimId::Qy;
  constexpr static auto Qz = DimId::Qz;
  constexpr static auto QSquared = DimId::QSquared;
  constexpr static auto Row = DimId::Row;
  constexpr static auto ScatteringAngle = DimId::ScatteringAngle;
  constexpr static auto Spectrum = DimId::Spectrum;
  constexpr static auto Temperature = DimId::Temperature;
  constexpr static auto Time = DimId::Time;
  constexpr static auto Tof = DimId::Tof;
  constexpr static auto Wavelength = DimId::Wavelength;
  constexpr static auto X = DimId::X;
  constexpr static auto Y = DimId::Y;
  constexpr static auto Z = DimId::Z;

  constexpr Dim() : m_id(DimId::Invalid) {}
  constexpr Dim(const DimId id) : m_id(id) {}
  explicit Dim(const std::string &label) {
    // Note that this is not thread-safe yet.
    for (const auto &ids : {builtin_ids, custom_ids})
      if (const auto it = ids.find(label); it != ids.end()) {
        m_id = it->second;
        return;
      }
    m_id = static_cast<DimId>(1000 + custom_ids.size());
    custom_ids[label] = m_id;
  }

  constexpr DimId id() const noexcept { return m_id; }

  std::string name() const {
    if (static_cast<int64_t>(m_id) < 1000)
      for (const auto &item : builtin_ids)
        if (item.second == m_id)
          return item.first;
    for (const auto &item : custom_ids)
      if (item.second == m_id)
        return item.first;
    return "unreachable"; // throw or terminate?
  }

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
  DimId m_id;
  static std::unordered_map<std::string, DimId> builtin_ids;
  static std::unordered_map<std::string, DimId> custom_ids;
};

std::string to_string(const Dim dim);

} // namespace scipp::units

// Hashing required temporarily while we use Dim as a key for the coord dict.
namespace std {
template <> struct hash<scipp::units::Dim> {
  std::size_t operator()(const scipp::units::Dim &k) const {
    return hash<scipp::units::DimId>()(k.id());
  }
};
} // namespace std

#endif // SCIPP_UNITS_DIM_H
