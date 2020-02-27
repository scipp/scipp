// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset_access.h"
#include "scipp/core/axis.h"
#include "scipp/core/dataset.h"

namespace scipp::core {

void DataArrayCoordAccess::set(const Dim &key, Variable var) const {
  m_parent->setCoord(key, std::move(var));
}
void DataArrayCoordAccess::set(const Dim &key, DataArrayAxis axis) const {
  m_parent->setCoord(key, std::move(axis));
}
void DataArrayCoordAccess::erase(const Dim &key) const {
  m_parent->eraseCoord(key);
}

void DatasetCoordAccess::set(const Dim &key, Variable var) const {
  m_parent->setCoord(key, std::move(var));
}
void DatasetCoordAccess::set(const Dim &key, DatasetAxis axis) const {
  m_parent->setCoord(key, std::move(axis));
}
void DatasetCoordAccess::erase(const Dim &key) const {
  m_parent->eraseCoord(key);
}

void MaskAccess::set(const std::string &key, Variable var) const {
  m_parent->setMask(key, std::move(var));
}
void MaskAccess::erase(const std::string &key) const {
  m_parent->eraseMask(key);
}

void AttrAccess::set(const std::string &key, Variable var) const {
  m_parent->setAttr(key, std::move(var));
}
void AttrAccess::erase(const std::string &key) const {
  m_parent->eraseAttr(key);
}

} // namespace scipp::core
