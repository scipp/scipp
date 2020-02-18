// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @author Simon Heybrock
#ifndef SCIPP_UNITS_DUMMY_H
#define SCIPP_UNITS_DUMMY_H

#include "scipp/units/unit.h"

namespace scipp::units {
namespace next {
using DimId = scipp::units::Dim;

class Dim {
public:
  constexpr static auto Invalid = DimId::Invalid;
  constexpr static auto X = DimId::X;
  constexpr static auto Y = DimId::Y;
  constexpr static auto Z = DimId::Z;

  Dim(const DimId id) : m_id(id) {}
  explicit Dim(const std::string &label) {
    if (const auto it = custom_ids.find(label); it != custom_ids.end())
      m_id = it->second;
    else {
      m_id = static_cast<DimId>(1000 + custom_ids.size());
      custom_ids[label] = m_id;
    }
  }

  std::string name() const {
    if (static_cast<int64_t>(m_id) < 1000)
      return to_string(m_id);
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

private:
  DimId m_id;
  static std::unordered_map<std::string, DimId> custom_ids;
  static int32_t custom_count;
};

std::unordered_map<std::string, DimId> Dim::custom_ids;
int32_t Dim::custom_count{0};

std::string to_string(const Dim dim) { return dim.name(); }

} // namespace next

} // namespace scipp::units

#endif // SCIPP_UNITS_DUMMY_H
