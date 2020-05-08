// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/map_view.h"

namespace scipp::dataset {

template <class Id, class Key, class Value>
ConstView<Id, Key, Value>::ConstView(holder_type &&items,
                                     const detail::slice_list &slices)
    : m_items(std::move(items)), m_slices(slices) {}

template <class Id, class Key, class Value>
bool ConstView<Id, Key, Value>::operator==(const ConstView &other) const {
  if (size() != other.size())
    return false;
  for (const auto &[name, data] : *this) {
    try {
      if (data != other[name])
        return false;
    } catch (except::NotFoundError &) {
      return false;
    }
  }
  return true;
}

template <class Id, class Key, class Value>
bool ConstView<Id, Key, Value>::operator!=(const ConstView &other) const {
  return !operator==(other);
}

template <class Base, class Access>
MutableView<Base, Access>::MutableView(const Access &access,
                                       typename Base::holder_type &&items,
                                       const detail::slice_list &slices)
    : Base(std::move(items), slices), m_access(access) {}

template <class Base, class Access>
MutableView<Base, Access>::MutableView(const Access &access, Base &&base)
    : Base(std::move(base)), m_access(access) {}

template class ConstView<ViewId::Coords, Dim, variable::Variable>;
template class MutableView<CoordsConstView, CoordAccess>;
template class ConstView<ViewId::Attrs, std::string, variable::Variable>;
template class MutableView<AttrsConstView, AttrAccess>;
template class ConstView<ViewId::Masks, std::string, variable::Variable>;
template class MutableView<MasksConstView, MaskAccess>;

} // namespace scipp::dataset
