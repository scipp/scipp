// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

#include "scipp/common/index.h"
#include "scipp/units/dim.h"

namespace scipp::sc_units {

namespace {
const auto &builtin_ids() {
  static std::unordered_map<std::string, Dim::Id> ids{
      {"<invalid>", Dim::Invalid},
      {"<none>", Dim::None},
      {"<internal_structure_component>", Dim::InternalStructureComponent},
      {"<internal_structure_row>", Dim::InternalStructureRow},
      {"<internal_structure_column>", Dim::InternalStructureColumn},
      {"<internal_histogram>", Dim::InternalHistogram},
      {"<internal_sort>", Dim::InternalSort},
      {"<internal_accumulate>", Dim::InternalAccumulate},
      {"energy", Dim::Energy},
      {"event", Dim::Event},
      {"group", Dim::Group},
      {"position", Dim::Position},
      {"row", Dim::Row},
      {"temperature", Dim::Temperature},
      {"time", Dim::Time},
      {"wavelength", Dim::Wavelength},
      {"x", Dim::X},
      {"y", Dim::Y},
      {"z", Dim::Z}};
  return ids;
}

auto &custom_ids() {
  static std::unordered_map<std::string, Dim::Id> ids;
  return ids;
}

std::shared_mutex mutex;
} // namespace

Dim::Dim(const std::string &label) {
  if (const auto it = builtin_ids().find(label); it != builtin_ids().end()) {
    m_id = it->second;
    return;
  }
  std::shared_lock read_lock(mutex);
  if (const auto it = custom_ids().find(label); it != custom_ids().end()) {
    m_id = it->second;
    return;
  }
  read_lock.unlock();
  const std::unique_lock write_lock(mutex);
  const auto id = scipp::size(custom_ids()) + 1000;
  if (id > std::numeric_limits<std::underlying_type<Id>::type>::max())
    throw std::runtime_error(
        "Exceeded maximum number of different dimension labels.");
  m_id = static_cast<Id>(id);
  custom_ids()[label] = m_id;
}

std::string Dim::name() const {
  if (static_cast<int64_t>(m_id) < 1000)
    for (const auto &item : builtin_ids())
      if (item.second == m_id)
        return item.first;
  const std::shared_lock read_lock(mutex);
  for (const auto &item : custom_ids())
    if (item.second == m_id)
      return item.first;
  return "unreachable"; // throw or terminate?
}

std::string to_string(const Dim dim) { return dim.name(); }

} // namespace scipp::sc_units
