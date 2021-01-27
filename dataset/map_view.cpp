// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/map_view.h"
#include "scipp/dataset/except.h"

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

/// Returns whether a given key is present in the view.
template <class Id, class Key, class Value>
bool ConstView<Id, Key, Value>::contains(const Key &k) const {
  return m_items.find(k) != m_items.cend();
}

/// Returns 1 or 0, depending on whether key is present in the view or not.
template <class Id, class Key, class Value>
scipp::index ConstView<Id, Key, Value>::count(const Key &k) const {
  return static_cast<scipp::index>(contains(k));
}

/// Return a const view to the coordinate for given dimension.
template <class Id, class Key, class Value>
typename ConstView<Id, Key, Value>::mapped_type::const_view_type
ConstView<Id, Key, Value>::operator[](const Key &key) const {
  scipp::expect::contains(*this, key);
  return make_slice(m_items.at(key), m_slices);
}

/// Return a const view to the coordinate for given dimension.
template <class Id, class Key, class Value>
typename ConstView<Id, Key, Value>::mapped_type::const_view_type
ConstView<Id, Key, Value>::at(const Key &key) const {
  return operator[](key);
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

/// Constructor for internal use (slicing and holding const view in mutable
/// view)
template <class Base, class Access>
MutableView<Base, Access>::MutableView(const Access &access, Base &&base)
    : Base(std::move(base)), m_access(access) {}

template <class Base, class Access>
/// Return a view to the coordinate for given dimension.
typename Base::mapped_type::view_type MutableView<Base, Access>::operator[](
    const typename Base::key_type &key) const {
  scipp::expect::contains(*this, key);
  return make_slice(Base::items().at(key), Base::slices());
}

template <class Base, class Access>
void MutableView<Base, Access>::erase(
    const typename Base::key_type &key) const {
  scipp::expect::contains(*this, key);
  m_access.erase(key);
}

template <class Base, class Access>
typename Base::mapped_type
MutableView<Base, Access>::extract(const typename Base::key_type &key) const {
  scipp::expect::contains(*this, key);
  return m_access.extract(key);
}

template class ConstView<ViewId::Coords, Dim, variable::Variable>;
template class MutableView<CoordsConstView, CoordAccess>;
template class ConstView<ViewId::Masks, std::string, variable::Variable>;
template class MutableView<MasksConstView, MaskAccess>;

} // namespace scipp::dataset
