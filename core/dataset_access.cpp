// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset_access.h"
#include "scipp/core/dataset.h"

namespace scipp::core {

namespace {
void expectValidParent(const Dataset *parent) {
  if (!parent)
    throw except::DatasetError("Cannot set or erase entry via a slice view.");
}
} // namespace

void CoordAccess::set(const Dim &key, Variable var) const {
  expectValidParent(m_parent);
  m_parent->setCoord(key, std::move(var));
}
void CoordAccess::erase(const Dim &key) const {
  expectValidParent(m_parent);
  m_parent->eraseCoord(key);
}

void MaskAccess::set(const std::string &key, Variable var) const {
  expectValidParent(m_parent);
  m_parent->setMask(key, std::move(var));
}
void MaskAccess::erase(const std::string &key) const {
  expectValidParent(m_parent);
  m_parent->eraseMask(key);
}

void AttrAccess::set(const std::string &key, Variable var) const {
  expectValidParent(m_parent);
  if (m_unaligned)
    m_unaligned->attrs().set(key, std::move(var));
  else if (m_name)
    m_parent->setAttr(*m_name, key, std::move(var));
  else
    m_parent->setAttr(key, std::move(var));
}
void AttrAccess::erase(const std::string &key) const {
  expectValidParent(m_parent);
  if (m_unaligned)
    m_unaligned->attrs().erase(key);
  else if (m_name)
    m_parent->eraseAttr(*m_name, key);
  else
    m_parent->eraseAttr(key);
}

} // namespace scipp::core
