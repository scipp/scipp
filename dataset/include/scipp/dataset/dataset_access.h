// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <string>

#include "scipp/units/dim.h"

namespace scipp::variable {
class Variable;
}

namespace scipp::dataset {

class DataArray;
class Dataset;

class CoordAccess {
public:
  CoordAccess(Dataset *parent, DataArray *unaligned = nullptr)
      : m_parent(parent), m_unaligned(unaligned) {}

  void set(const Dim &key, variable::Variable var) const;
  void erase(const Dim &key) const;

private:
  Dataset *m_parent;
  DataArray *m_unaligned;
};

class MaskAccess {
public:
  MaskAccess(Dataset *parent, DataArray *unaligned = nullptr)
      : m_parent(parent), m_unaligned(unaligned) {}

  void set(const std::string &key, variable::Variable var) const;
  void erase(const std::string &key) const;

private:
  Dataset *m_parent;
  DataArray *m_unaligned;
};

class AttrAccess {
public:
  AttrAccess(Dataset *parent, const std::string *name = nullptr,
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
