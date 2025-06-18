// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <sstream>

#include "scipp/core/except.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/dataset_util.h"
#include "scipp/dataset/except.h"
#include "scipp/units/unit.h"

namespace scipp::dataset {

namespace {
template <class T> void expect_writable(const T &dict) {
  if (dict.is_readonly())
    throw except::DatasetError(
        "Read-only flag is set, cannot insert new or erase existing items.");
}

void expect_valid(const Dataset &ds) {
  if (!ds.is_valid())
    throw except::DatasetError(
        "Dataset is not valid. This is an internal error stemming from an "
        "improperly initialized dataset.");
}

template <class T>
void expect_matching_item_dims(const Dataset &dset, const std::string_view key,
                               const T &to_insert) {
  if (dset.sizes() != to_insert.dims()) {
    std::ostringstream msg;
    msg << "Cannot add item '" << key << "' with dims " << to_insert.dims()
        << " to dataset with dims " << to_string(dset.sizes()) << ".";
    throw except::DimensionError(msg.str());
  }
}
} // namespace

/// Make an invalid dataset.
///
/// Such a dataset is intended to be filled using setDataInit and must
/// never be exposed to Python!
Dataset::Dataset() : m_valid{false} {}

Dataset::Dataset(const Dataset &other)
    : m_coords(other.m_coords), m_data(other.m_data), m_readonly(false),
      m_valid{other.m_valid} {}

Dataset::Dataset(const DataArray &data) {
  m_coords.setSizes(data.dims());
  setData(data.name(), data);
}

Dataset &Dataset::operator=(const Dataset &other) {
  return *this = Dataset(other);
}

Dataset &Dataset::operator=(Dataset &&other) {
  if (this == &other) {
    return *this;
  }
  check_nested_in_assign(*this, other);
  m_coords = std::move(other.m_coords);
  m_data = std::move(other.m_data);
  m_readonly = other.m_readonly;
  m_valid = other.m_valid;
  return *this;
}

/// Removes all data items from the Dataset.
///
/// Coordinates are not modified.
void Dataset::clear() {
  expect_writable(*this);
  m_data.clear();
}

void Dataset::setCoords(Coords other) {
  expect_valid(*this);
  scipp::expect::includes(other.sizes(), m_coords.sizes());
  m_coords = std::move(other);
}
/// Return a const view to all coordinates of the dataset.
const Coords &Dataset::coords() const noexcept { return m_coords; }

/// Return a view to all coordinates of the dataset.
Coords &Dataset::coords() noexcept { return m_coords; }

/// Return a Dataset without the given coordinate names.
Dataset Dataset::drop_coords(const std::span<const Dim> coord_names) const {
  Dataset result = *this;
  for (const auto &name : coord_names)
    result.coords().erase(name);
  return result;
}

bool Dataset::contains(const std::string &name) const noexcept {
  return m_data.contains(name);
}

/// Removes a data item from the Dataset
///
/// Coordinates are not modified.
void Dataset::erase(const std::string &name) {
  expect_writable(*this);
  scipp::expect::contains(*this, name);
  m_data.erase(std::string(name));
}

/// Extract a data item from the Dataset, returning a DataArray
///
/// Coordinates are not modified.
DataArray Dataset::extract(const std::string &name) {
  auto extracted = operator[](name);
  erase(name);
  return extracted;
}

/// Return a data item with coordinates with given name.
DataArray Dataset::operator[](const std::string &name) const {
  scipp::expect::contains(*this, name);
  return *find(name);
}

/// Set (insert or replace) the coordinate for the given dimension.
void Dataset::setCoord(const Dim dim, Variable coord) {
  expect_writable(*this);
  expect_valid(*this);
  m_coords.set(dim, std::move(coord));
}

/// Set (insert or replace) data (values, optional variances) with given name.
///
/// Throws if the provided values bring the dataset into an inconsistent state
/// (mismatching dimensions).
void Dataset::setData(const std::string &name, Variable data) {
  expect_writable(*this);
  expect_valid(*this);
  expect_matching_item_dims(*this, name, data);
  m_data.insert_or_assign(name, DataArray(std::move(data)));
}

// See docs of overload for data arrays.
void Dataset::setDataInit(const std::string &name, Variable data) {
  if (!is_valid()) {
    m_coords.setSizes(data.dims());
    m_valid = true;
  }
  setData(name, std::move(data));
}

namespace {
auto coords_to_skip(const Dataset &dst, const DataArray &src) {
  std::vector<Dim> to_skip;
  for (auto &&[dim, coord] : src.coords())
    if (const auto it = dst.coords().find(dim); it != dst.coords().end()) {
      if (it->second.is_aligned() == coord.is_aligned())
        expect::matching_coord(dim, coord, it->second, "set coord");
      else if (it->second.is_aligned())
        // Aligned coords overwrite unaligned.
        to_skip.push_back(dim);
    }
  return to_skip;
}
} // namespace

/// Set (insert or replace) data from a DataArray with a given name.
///
/// Coordinates and masks of the data array are added to the dataset.
/// Throws if there are existing but mismatching coords or masks.
/// Throws if the provided data brings the dataset into an
/// inconsistent state (mismatching dimensions).
void Dataset::setData(const std::string &name, const DataArray &data) {
  // Return early on self assign to avoid exceptions from Python inplace ops
  if (const auto it = find(name); it != end()) {
    if (it->data().is_same(data.data()) && it->masks() == data.masks() &&
        it->coords() == data.coords())
      return;
  }
  const auto to_skip = coords_to_skip(*this, data);

  setData(name, data.data());
  auto &item = m_data[name];
  for (auto &&[dim, coord] : data.coords())
    if (m_coords.find(dim) == m_coords.end() &&
        std::find(to_skip.begin(), to_skip.end(), dim) == to_skip.end())
      setCoord(dim, coord);
  item.masks() = data.masks();
}

/// Like setData but allow inserting into a default-initialized dataset.
///
/// A default-constructed dataset cannot be filled using setData or setCoord
/// as the dataset's dimensions are unknown and the input cannot be validated.
/// setDataInit sets the sizes when called with a default-initialized dataset.
/// It can be used for creating a new dataset and filling it step by step.
///
/// When using this, always make sure to ultimately produce a valid dataset.
/// setDataInit is often called in a loop.
/// Keep in mind that the loop might not run when the input dataset is empty!
void Dataset::setDataInit(const std::string &name, const DataArray &data) {
  if (!is_valid()) {
    m_coords.setSizes(data.dims());
    m_valid = true;
  }
  setData(name, data);
}

/// Return slice of the dataset along given dimension with given extents.
Dataset Dataset::slice(const Slice &s) const {
  Dataset out(slice_map(m_coords.sizes(), m_data, s));
  out.m_coords = m_coords.slice_coords(s);
  out.m_readonly = true;
  return out;
}

Dataset &Dataset::setSlice(const Slice &s, const Dataset &data) {
  // Validate slice on all items as a dry-run
  expect::coords_are_superset(slice(s).coords(), data.coords(), "");
  for (const auto &[name, item] : m_data)
    item.validateSlice(s, data.m_data.at(name));
  // Only if all items checked for dry-run does modification go-ahead
  for (auto &&[name, item] : m_data)
    item.setSlice(s, data.m_data.at(name));
  return *this;
}

Dataset &Dataset::setSlice(const Slice &s, const DataArray &data) {
  // Validate slice on all items as a dry-run
  expect::coords_are_superset(slice(s).coords(), data.coords(), "");
  for (const auto &item : m_data)
    item.second.validateSlice(s, data);
  // Only if all items checked for dry-run does modification go-ahead
  for (auto &&[_, val] : m_data)
    val.setSlice(s, data);
  return *this;
}

Dataset &Dataset::setSlice(const Slice &s, const Variable &data) {
  for (auto &&item : *this)
    item.setSlice(s, data);
  return *this;
}

/// Rename dimension `from` to `to`.
Dataset
Dataset::rename_dims(const std::vector<std::pair<Dim, Dim>> &names) const {
  Dataset out({}, m_coords.rename_dims(names));
  for (const auto &[name, da] : m_data)
    out.setData(name, da.rename_dims(names, false));
  return out;
}

/// Return true if the datasets have identical content.
bool Dataset::operator==(const Dataset &other) const {
  if (size() != other.size())
    return false;
  if (coords() != other.coords())
    return false;
  for (const auto &data : *this)
    if (!other.contains(data.name()) || data != other[data.name()])
      return false;
  return true;
}

/// Return true if the datasets have mismatching content./
bool Dataset::operator!=(const Dataset &other) const {
  return !operator==(other);
}

bool equals_nan(const Dataset &a, const Dataset &b) {
  if (a.size() != b.size())
    return false;
  if (!equals_nan(a.coords(), b.coords()))
    return false;
  for (const auto &data : a)
    if (!b.contains(data.name()) || !equals_nan(data, b[data.name()]))
      return false;
  return true;
}

const Sizes &Dataset::sizes() const { return m_coords.sizes(); }
const Sizes &Dataset::dims() const { return sizes(); }
Dim Dataset::dim() const {
  core::expect::ndim_is(sizes(), 1);
  return *sizes().begin();
}
scipp::index Dataset::ndim() const { return scipp::size(m_coords.sizes()); }

bool Dataset::is_readonly() const noexcept { return m_readonly; }

bool Dataset::is_valid() const noexcept { return m_valid; }

typename Masks::holder_type union_or(const Masks &currentMasks,
                                     const Masks &otherMasks) {
  typename Masks::holder_type out;

  for (const auto &[key, item] : currentMasks)
    out.insert_or_assign(key, copy(item));
  for (const auto &[key, item] : otherMasks) {
    if (!currentMasks.contains(key)) {
      out.insert_or_assign(key, copy(item));
    } else if (item.dtype() != core::dtype<bool> ||
               out[key].dtype() != core::dtype<bool>) {
      throw except::TypeError(" Cannot combine non-boolean mask '" + key +
                              "' in operation");
    } else if (item.unit() != scipp::sc_units::none ||
               out[key].unit() != scipp::sc_units::none) {
      throw except::UnitError(" Cannot combine a unit-specified mask '" + key +
                              "' in operation");
    } else if (out[key].dims().includes(item.dims())) {
      out[key] |= item;
    } else {
      out[key] = out[key] | item;
    }
  }
  return out;
}

void union_or_in_place(Masks &currentMasks, const Masks &otherMasks) {
  using core::to_string;
  using sc_units::to_string;

  for (const auto &[key, item] : otherMasks) {
    const auto it = currentMasks.find(key);
    if (it == currentMasks.end() && currentMasks.is_readonly()) {
      throw except::NotFoundError("Cannot insert new mask '" + to_string(key) +
                                  "' via a slice.");
    } else if (it != currentMasks.end() && it->second.is_readonly() &&
               it->second != (it->second | item)) {
      throw except::DimensionError("Cannot update mask '" + to_string(key) +
                                   "' via slice since the mask is implicitly "
                                   "broadcast along the slice dimension.");
    } else if (it != currentMasks.end() &&
               (item.dtype() != core::dtype<bool> ||
                currentMasks[key].dtype() != core::dtype<bool>)) {
      throw except::TypeError(" Cannot combine non-boolean mask '" + key +
                              "' in operation");
    } else if (it != currentMasks.end() &&
               (item.unit() != scipp::sc_units::none ||
                currentMasks[key].unit() != scipp::sc_units::none)) {
      throw except::UnitError(" Cannot combine a unit-specified mask '" + key +
                              "' in operation");
    }
  }

  for (const auto &[key, item] : otherMasks) {
    const auto it = currentMasks.find(key);
    if (it == currentMasks.end()) {
      currentMasks.set(key, copy(item));
    } else if (!it->second.is_readonly()) {
      it->second |= item;
    }
  }
}

} // namespace scipp::dataset
