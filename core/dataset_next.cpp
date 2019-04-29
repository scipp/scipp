// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dataset_next.h"
#include "dataset.h"
#include "except.h"

namespace scipp::core::next {

CoordsConstProxy Dataset::coords() const noexcept {
  return CoordsConstProxy(*this);
}

DataConstProxy Dataset::operator[](const std::string &name) const {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::runtime_error("Could not find data with name " + name + ".");
  // Note that we are passing it->first instead of name to avoid storing a
  // reference to a temporary.
  return DataConstProxy(this, it->first);
}

ConstVariableSlice CoordsConstProxy::operator[](const Dim dim) const {
  return ConstVariableSlice(*m_coords.at(dim));
}

// ConstVariableSlice LabelsConstProxy::operator[](const std::string &name)
// const {
//  return ConstVariableSlice{m_dataset->m_labels.at(name)};
//}

} // namespace scipp::core::next
