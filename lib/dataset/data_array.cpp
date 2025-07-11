// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/data_array.h"
#include "scipp/dataset/dataset_util.h"
#include "scipp/variable/misc_operations.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

namespace {
template <class T> void expect_writable(const T &dict) {
  if (dict.is_readonly())
    throw except::DataArrayError("Read-only flag is set, cannot set new data.");
}

template <class T> auto copy_shared(const std::shared_ptr<T> &obj) {
  return obj ? std::make_shared<T>(*obj) : obj;
}
} // namespace

DataArray::DataArray(const DataArray &other)
    : m_name(other.m_name), m_data(copy_shared(other.m_data)),
      m_coords(copy_shared(other.m_coords)),
      m_masks(copy_shared(other.m_masks)), m_readonly(false) {}

DataArray::DataArray(Variable data, Coords coords, Masks masks,
                     const std::string_view name)
    : m_name(name), m_data(std::make_shared<Variable>(std::move(data))),
      m_coords(std::make_shared<Coords>(std::move(coords))),
      m_masks(std::make_shared<Masks>(std::move(masks))) {
  const Sizes sizes(dims());
  m_coords->setSizes(sizes);
  m_masks->setSizes(sizes);
}

DataArray::DataArray(Variable data, typename Coords::holder_type coords,
                     typename Masks::holder_type masks,
                     const std::string_view name)
    : m_name(name), m_data(std::make_shared<Variable>(std::move(data))),
      m_coords(std::make_shared<Coords>(dims(), std::move(coords))),
      m_masks(std::make_shared<Masks>(dims(), std::move(masks))) {}

DataArray &DataArray::operator=(const DataArray &other) {
  if (this == &other) {
    return *this;
  }
  check_nested_in_assign(*this, other);
  return *this = DataArray(other);
}

DataArray &DataArray::operator=(DataArray &&other) {
  if (this == &other) {
    return *this;
  }
  check_nested_in_assign(*this, other);
  m_name = std::move(other.m_name);
  m_data = std::move(other.m_data);
  m_coords = std::move(other.m_coords);
  m_masks = std::move(other.m_masks);
  m_readonly = other.m_readonly;
  return *this;
}

void DataArray::setData(const Variable &data) {
  // Return early on self assign to avoid exceptions from Python inplace ops
  if (m_data->is_same(data))
    return;
  expect_writable(*this);
  core::expect::equals(static_cast<Sizes>(dims()),
                       static_cast<Sizes>(data.dims()));
  *m_data = data;
}

/// Return true if the dataset proxies have identical content.
bool operator==(const DataArray &a, const DataArray &b) {
  if (a.has_variances() != b.has_variances())
    return false;
  if (a.coords() != b.coords())
    return false;
  if (a.masks() != b.masks())
    return false;
  return a.data() == b.data();
}

bool operator!=(const DataArray &a, const DataArray &b) {
  return !operator==(a, b);
}

bool equals_nan(const DataArray &a, const DataArray &b) {
  if (!equals_nan(a.coords(), b.coords()))
    return false;
  if (!equals_nan(a.masks(), b.masks()))
    return false;
  return equals_nan(a.data(), b.data());
}

/// Return the name of the data array.
///
/// If part of a dataset, the name of the array is equal to the key of this item
/// in the dataset. Note that comparison operations ignore the name.
const std::string &DataArray::name() const { return m_name; }

void DataArray::setName(const std::string_view name) { m_name = name; }

DataArray DataArray::slice(const Slice &s) const {
  auto out = DataArray{m_data->slice(s), m_coords->slice_coords(s),
                       m_masks->slice(s), m_name};
  out.m_readonly = true;
  return out;
}

void DataArray::validateSlice(const Slice &s, const DataArray &array) const {
  expect::coords_are_superset(slice(s), array, "");
  data().validateSlice(s, array.data());
  masks().validateSlice(s, array.masks());
}

DataArray &DataArray::setSlice(const Slice &s, const DataArray &array) {
  // validateSlice, but not masks as otherwise repeated
  expect::coords_are_superset(slice(s), array, "");
  data().validateSlice(s, array.data());
  // Apply changes
  masks().setSlice(s, array.masks());
  return setSlice(s, array.data());
}

DataArray &DataArray::setSlice(const Slice &s, const Variable &var) {
  data().setSlice(s, var);
  return *this;
}

DataArray DataArray::view() const {
  DataArray out;
  out.m_data = m_data;     // share data
  out.m_coords = m_coords; // share coords
  out.m_masks = m_masks;   // share masks
  out.m_name = m_name;
  return out;
}

DataArray DataArray::view_with_coords(const Coords &coords,
                                      const std::string &name,
                                      const bool readonly) const {
  DataArray out;
  out.m_data = m_data; // share data
  const Sizes sizes(dims());
  typename Coords::holder_type selected;
  for (const auto &[dim, coord] : coords)
    if (coords.item_applies_to(dim, dims()))
      selected.insert_or_assign(dim, coord.as_const());
  const bool readonly_coords = true;
  out.m_coords =
      std::make_shared<Coords>(sizes, std::move(selected), readonly_coords);
  out.m_masks = m_masks; // share masks
  out.m_name = name;
  out.m_readonly = readonly;
  return out;
}

DataArray DataArray::drop_coords(const std::span<const Dim> coord_names) const {
  DataArray result = *this;
  for (const auto &name : coord_names)
    result.coords().erase(name);
  return result;
}

DataArray
DataArray::drop_masks(const std::span<const std::string> mask_names) const {
  DataArray result = *this;
  for (const auto &name : mask_names)
    result.masks().erase(name);
  return result;
}

DataArray DataArray::rename_dims(const std::vector<std::pair<Dim, Dim>> &names,
                                 const bool fail_on_unknown) const {
  return DataArray(m_data->rename_dims(names, fail_on_unknown),
                   m_coords->rename_dims(names, false),
                   m_masks->rename_dims(names, false));
}

DataArray DataArray::as_const() const {
  auto out = DataArray(data().as_const(), coords().as_const(),
                       masks().as_const(), name());
  out.m_readonly = true;
  return out;
}

bool DataArray::is_readonly() const noexcept { return m_readonly; }

} // namespace scipp::dataset
