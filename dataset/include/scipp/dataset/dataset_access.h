// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <string>

#include "scipp-dataset_export.h"
#include "scipp/common/deep_ptr.h"
#include "scipp/units/dim.h"

namespace scipp::variable {
class Variable;
}

namespace scipp::dataset {

class DataArray;
class Dataset;

class CoordAccess {
public:
  CoordAccess(Dataset *parent, const std::string *name = nullptr,
              deep_ptr<CoordAccess> &&unaligned = nullptr)
      : m_parent(parent), m_name(name), m_unaligned(std::move(unaligned)) {}

  void set(const Dim &key, variable::Variable var) const;
  void erase(const Dim &key) const;

private:
  Dataset *m_parent;
  const std::string *m_name;
  deep_ptr<CoordAccess> m_unaligned;
};

class MaskAccess {
public:
  MaskAccess(Dataset *parent, const std::string *name,
             DataArray *unaligned = nullptr)
      : m_parent(parent), m_name(name), m_unaligned(unaligned) {}

  void set(const std::string &key, variable::Variable var) const;
  void erase(const std::string &key) const;

private:
  Dataset *m_parent;
  const std::string *m_name;
  DataArray *m_unaligned;
};

} // namespace scipp::dataset
