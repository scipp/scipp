// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/data_array.h"
#include "scipp/variable/misc_operations.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

namespace {
template <class T> auto copy_shared(const std::shared_ptr<T> &obj) {
  return std::make_shared<T>(*obj);
}
} // namespace

DataArray::DataArray(const DataArray &other, const AttrPolicy attrPolicy)
    : m_name(other.m_name), m_data(other.m_data), m_coords(other.m_coords),
      m_masks(copy_shared(other.m_masks)),
      m_attrs(attrPolicy == AttrPolicy::Keep ? copy_shared(other.m_attrs)
                                             : std::make_shared<Attrs>()) {}

DataArray::DataArray(const DataArray &other)
    : DataArray(other, AttrPolicy::Keep) {}

DataArray::DataArray(Variable data, Coords coords, Masks masks, Attrs attrs,
                     const std::string &name)
    : m_name(name), m_data(std::move(data)), m_coords(std::move(coords)),
      m_masks(std::make_shared<Masks>(std::move(masks))),
      m_attrs(std::make_shared<Attrs>(std::move(attrs))) {
  const Sizes sizes(dims());
  core::expect::equals(sizes, m_coords.sizes());
  core::expect::equals(sizes, m_masks->sizes());
  core::expect::equals(sizes, m_attrs->sizes());
}

DataArray::DataArray(Variable data, typename Coords::holder_type coords,
                     typename Masks::holder_type masks,
                     typename Attrs::holder_type attrs, const std::string &name)
    : m_name(name), m_data(std::move(data)),
      m_coords(dims(), std::move(coords)),
      m_masks(std::make_shared<Masks>(dims(), std::move(masks))),
      m_attrs(std::make_shared<Attrs>(dims(), std::move(attrs))) {}

DataArray &DataArray::operator=(const DataArray &other) {
  return *this = DataArray(other);
}

void DataArray::setData(Variable data) {
  core::expect::equals(dims(), data.dims());
  m_data = data;
}

/// Return true if the dataset proxies have identical content.
bool operator==(const DataArray &a, const DataArray &b) {
  if (a.hasVariances() != b.hasVariances())
    return false;
  if (a.coords() != b.coords())
    return false;
  if (a.masks() != b.masks())
    return false;
  if (a.attrs() != b.attrs())
    return false;
  return a.data() == b.data();
}

bool operator!=(const DataArray &a, const DataArray &b) {
  return !operator==(a, b);
}

/// Return the name of the data array.
///
/// If part of a dataset, the name of the array is equal to the key of this item
/// in the dataset. Note that comparison operations ignore the name.
const std::string &DataArray::name() const { return m_name; }

void DataArray::setName(const std::string &name) { m_name = name; }

DataArray DataArray::slice(const Slice &s) const {
  DataArray out{m_data.slice(s), m_coords.slice(s), m_masks->slice(s),
                m_attrs->slice(s), m_name};
  for (auto it = out.m_coords.begin(); it != out.m_coords.end();) {
    if (unaligned_by_dim_slice(*it, s.dim())) {
      out.attrs().set(it->first, it->second);
      out.m_coords.erase(it->first);
    }
    ++it;
  }
  return out;
}

DataArray DataArray::view_with_coords(const Coords &coords) const {
  // TODO also handle name here? should be set from dataset
  DataArray out;
  out.m_data = m_data;
  out.m_coords = Coords(m_data.dims(), {});
  // TODO bin edge handling
  for (const auto &[dim, coord] : coords)
    if (m_data.dims().contains(coord.dims()))
      out.m_coords.set(dim, coord);
  out.m_masks = m_masks; // share masks
  out.m_attrs = m_attrs; // share attrs
  return out;
}

void DataArray::rename(const Dim from, const Dim to) {
  if ((from != to) && dims().contains(to))
    throw except::DimensionError("Duplicate dimension.");

  m_data.rename(from, to);
  const auto relabel = [from, to](auto &map) {
    auto node = map.extract(from);
    node.key() = to;
    map.insert(std::move(node));
  };
  if (m_coords.count(from))
    relabel(m_coords.items());
  for (auto &item : m_coords)
    item.second.rename(from, to);
  for (auto &item : *m_masks)
    item.second.rename(from, to);
  for (auto &item : *m_attrs)
    item.second.rename(from, to);
}

DataArray astype(const DataArray &var, const DType type) {
  return DataArray(astype(var.data(), type), var.coords(), var.masks(),
                   var.attrs(), var.name());
}

} // namespace scipp::dataset
