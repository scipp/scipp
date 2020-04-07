// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset_access.h"
#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

namespace {

void expectValidParent(const Dataset *parent) {
  if (!parent)
    throw except::DatasetError("Cannot set or erase entry via a slice view.");
}

void expectDimsNotContained(const Dataset *parent, const Variable &var) {
  const auto &dims = parent->dimensions();
  const auto &labels = var.dims().labels();
  if (std::any_of(labels.begin(), labels.end(),
                  [&dims](const Dim dim) { return dims.count(dim) == 0; }))
    return;
  throw except::RealignedDataError("set in realigned, not unaligned.");
}

auto clarify_exception(const except::NotFoundError &e) {
  return except::NotFoundError(
      std::string(e.what()) +
      " This may be because of an attempt to remove a coord/masks/attr via "
      "the `unaligned` property of realigned data. Try removing from the "
      "realigned parent.");
}

} // namespace

void CoordAccess::set(const Dim &key, Variable var) const {
  expectValidParent(m_parent);
  if (m_unaligned) {
    expectDimsNotContained(m_parent, var);
    m_unaligned->coords().set(key, std::move(var));
  } else
    m_parent->setCoord(key, std::move(var));
}
void CoordAccess::erase(const Dim &key) const {
  expectValidParent(m_parent);
  if (m_unaligned)
    try {
      m_unaligned->coords().erase(key);
    } catch (const except::NotFoundError &e) {
      throw clarify_exception(e);
    }
  else
    m_parent->eraseCoord(key);
}

void MaskAccess::set(const std::string &key, Variable var) const {
  expectValidParent(m_parent);
  if (m_unaligned) {
    expectDimsNotContained(m_parent, var);
    m_unaligned->masks().set(key, std::move(var));
  } else {
    m_parent->setMask(key, std::move(var));
  }
}
void MaskAccess::erase(const std::string &key) const {
  expectValidParent(m_parent);
  if (m_unaligned)
    try {
      m_unaligned->masks().erase(key);
    } catch (const except::NotFoundError &e) {
      throw clarify_exception(e);
    }
  else {
    m_parent->eraseMask(key);
  }
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

} // namespace scipp::dataset
