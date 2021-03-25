// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/data_array.h"
#include "scipp/variable/misc_operations.h"

//#include "dataset_operations_common.h"

namespace scipp::dataset {

/*
DataArray::DataArray(const DataArray &view,
                   const AttrPolicy attrPolicy)
  : DataArray(Variable(view.data()), copy_map(view.coords()),
              copy_map(view.masks()),
              attrPolicy == AttrPolicy::Keep ? copy_map(view.attrs())
                                             : std::map<Dim, Variable>{},
              view.name()) {}
              */
namespace {
template <class T> auto copy_shared(const std::shared_ptr<T> &obj) {
  return std::make_shared<T>(*obj);
}
} // namespace

DataArray::DataArray(const DataArray &other)
    : m_name(other.m_name), m_data(other.m_data), m_coords(other.m_coords),
      m_masks(copy_shared(other.m_masks)), m_attrs(copy_shared(other.m_attrs)) {
}

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

void DataArray::setData(Variable data) {
  core::expect::equals(dims(), data.dims());
  m_data = data;
}

DataArray &DataArray::operator=(const DataArray &other) {
  return *this = DataArray(other);
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

DataArray astype(const DataArray &var, const DType type) {
  return DataArray(astype(var.data(), type), var.coords(), var.masks(),
                   var.attrs(), var.name());
}

} // namespace scipp::dataset
