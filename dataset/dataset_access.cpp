// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset_access.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

namespace scipp::dataset {

namespace {
void expectValidParent(const Dataset *parent) {
  if (!parent)
    throw except::DatasetError("Cannot set or erase entry via a slice view.");
}
} // namespace

void CoordAccess::set(const Dim &key, Variable var) const {
  expectValidParent(m_parent);
  if (m_name && m_isItem) {
    m_parent->setCoord(*m_name, key, std::move(var));
  } else
    m_parent->setCoord(key, std::move(var));
}
void CoordAccess::erase(const Dim &key) const { extract(key); }
Variable CoordAccess::extract(const Dim &key) const {
  expectValidParent(m_parent);
  if (m_name) {
    if (!m_isItem && m_parent->coords().contains(key))
      return m_parent->extractCoord(
          key); // this is a DataArray, may delete aligned
    else
      return m_parent->extractCoord(*m_name, key);
  } else
    return m_parent->extractCoord(key);
}

void MaskAccess::set(const std::string &key, Variable var) const {
  expectValidParent(m_parent);
  m_parent->setMask(*m_name, key, std::move(var));
}
void MaskAccess::erase(const std::string &key) const { extract(key); }
Variable MaskAccess::extract(const std::string &key) const {
  expectValidParent(m_parent);
  return m_parent->extractMask(*m_name, key);
}

} // namespace scipp::dataset
