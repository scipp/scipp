// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DATASET_ACCESS_H
#define SCIPP_CORE_DATASET_ACCESS_H

#include "scipp/core/axis_forward.h"
#include "scipp/core/variable.h"

namespace scipp::core {

class DataArray;
class Dataset;

class DataArrayCoordAccess {
public:
  DataArrayCoordAccess(DataArray *parent) : m_parent(parent) {}

  void set(const Dim &key, Variable var) const;
  void set(const Dim &key, DataArrayAxis axis) const;
  void erase(const Dim &key) const;

private:
  DataArray *m_parent;
};

class DatasetCoordAccess {
public:
  DatasetCoordAccess(Dataset *parent) : m_parent(parent) {}

  void set(const Dim &key, Variable var) const;
  void set(const Dim &key, DatasetAxis axis) const;
  void erase(const Dim &key) const;

private:
  Dataset *m_parent;
};

class MaskAccess {
public:
  MaskAccess(Dataset *parent) : m_parent(parent) {}

  void set(const std::string &key, Variable var) const;
  void erase(const std::string &key) const;

private:
  Dataset *m_parent;
};

class AttrAccess {
public:
  AttrAccess(Dataset *parent) : m_parent(parent) {}

  void set(const std::string &key, Variable var) const;
  void erase(const std::string &key) const;

private:
  Dataset *m_parent;
};

} // namespace scipp::core

#endif // SCIPP_CORE_DATASET_ACCESS_H
