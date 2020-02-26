// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_UNALIGNED_ACCESS_H
#define SCIPP_CORE_UNALIGNED_ACCESS_H

#include "scipp-core_export.h"
#include "scipp/core/variable.h"

namespace scipp::core {

class DatasetAxis;

class SCIPP_CORE_EXPORT UnalignedAccess {
public:
  using unaligned_type = std::unordered_map<std::string, Variable>;
  UnalignedAccess() = default;
  UnalignedAccess(DatasetAxis *parent, unaligned_type *unaligned)
      : m_parent(parent), m_unaligned(unaligned) {}

  void set(const std::string &key, Variable var) const;
  void erase(const std::string &key) const;

private:
  DatasetAxis *m_parent;
  unaligned_type *m_unaligned;
};

} // namespace scipp::core

#endif // SCIPP_CORE_UNALIGNED_ACCESS_H
