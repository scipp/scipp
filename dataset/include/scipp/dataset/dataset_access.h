// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
              const bool isItem = true)
      : m_parent(parent), m_name(name), m_isItem(isItem) {}

  void set(const Dim &key, variable::Variable var) const;
  void erase(const Dim &key) const;
  [[maybe_unused]] variable::Variable extract(const Dim &key) const;

private:
  Dataset *m_parent;
  const std::string *m_name;
  bool m_isItem;
};

class MaskAccess {
public:
  MaskAccess(Dataset *parent, const std::string *name)
      : m_parent(parent), m_name(name) {}

  void set(const std::string &key, variable::Variable var) const;
  void erase(const std::string &key) const;
  [[maybe_unused]] variable::Variable extract(const std::string &key) const;

private:
  Dataset *m_parent;
  const std::string *m_name;
};

} // namespace scipp::dataset
