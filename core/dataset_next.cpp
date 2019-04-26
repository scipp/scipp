// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dataset_next.h"
#include "dataset.h"
#include "except.h"

namespace scipp::core::next {

CoordsConstProxy Dataset::coords() const noexcept {
  return CoordsConstProxy(this);
}

ConstVariableSlice CoordsConstProxy::operator[](const Dim dim) const {
  return ConstVariableSlice{m_dataset->m_coords.at(dim)};
}

ConstVariableSlice LabelsConstProxy::operator[](const std::string &name) const {
  return ConstVariableSlice{m_dataset->m_userCoords.at(name)};
}

} // namespace scipp::core::next
