// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <string>
#include <string_view>

#include "scipp/dataset/map_view.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {

/// Policies for attribute propagation in operations with data arrays or
/// dataset.
enum class AttrPolicy { Keep, Drop };

/// Data array, a variable with coordinates, masks, and attributes.
class SCIPP_DATASET_EXPORT DataArray {
public:
  DataArray() = default;
  DataArray(const DataArray &other, AttrPolicy attrPolicy);
  DataArray(const DataArray &other);
  DataArray(DataArray &&other) = default;

  DataArray(Variable data, Coords coords, Masks masks, Attrs attrs,
            std::string_view name = "");
  explicit DataArray(Variable data, typename Coords::holder_type coords = {},
                     typename Masks::holder_type masks = {},
                     typename Attrs::holder_type attrs = {},
                     std::string_view name = "");

  DataArray &operator=(const DataArray &other);
  DataArray &operator=(DataArray &&other);

  bool is_valid() const noexcept { return m_data && m_data->is_valid(); }

  const std::string &name() const;
  void setName(std::string_view name);

  const Coords &coords() const { return *m_coords; }
  // TODO either ensure Dict does not allow changing sizes, or return by value
  // here
  Coords &coords() { return *m_coords; }

  const Masks &masks() const { return *m_masks; }
  Masks &masks() { return *m_masks; }

  const Attrs &attrs() const { return *m_attrs; }
  Attrs &attrs() { return *m_attrs; }

  Coords meta() const;

  [[nodiscard]] Dimensions dims() const { return m_data->dims(); }
  [[nodiscard]] Dim dim() const { return m_data->dim(); }
  [[nodiscard]] scipp::index ndim() const { return m_data->ndim(); }
  [[nodiscard]] scipp::span<const scipp::index> strides() const {
    return m_data->strides();
  }
  [[nodiscard]] DType dtype() const { return m_data->dtype(); }
  [[nodiscard]] units::Unit unit() const { return m_data->unit(); }

  void setUnit(const units::Unit unit) { m_data->setUnit(unit); }

  /// Return true if the data array contains data variances.
  bool has_variances() const { return m_data->has_variances(); }

  /// Return untyped const view for data (values and optional variances).
  const Variable &data() const { return *m_data; }
  /// Return untyped view for data (values and optional variances).
  Variable data() { return *m_data; }

  /// Return typed const view for data values.
  template <class T> auto values() const {
    return std::as_const(*m_data).values<T>();
  }
  /// Return typed view for data values.
  template <class T> auto values() { return m_data->values<T>(); }

  /// Return typed const view for data variances.
  template <class T> auto variances() const {
    return std::as_const(*m_data).variances<T>();
  }
  /// Return typed view for data variances.
  template <class T> auto variances() { return m_data->variances<T>(); }

  void rename(Dim from, Dim to);

  void setData(const Variable &data);

  DataArray slice(const Slice &s) const;
  void validateSlice(const Slice &s, const DataArray &array) const;
  [[maybe_unused]] DataArray &setSlice(const Slice &s, const DataArray &array);
  [[maybe_unused]] DataArray &setSlice(const Slice &s, const Variable &var);

  DataArray view() const;
  DataArray view_with_coords(const Coords &coords, const std::string &name,
                             const bool readonly) const;

  [[nodiscard]] DataArray as_const() const;

  bool is_readonly() const noexcept;

private:
  // Declared friend so gtest recognizes it
  friend SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &,
                                                       const DataArray &);
  std::string m_name;
  std::shared_ptr<Variable> m_data;
  std::shared_ptr<Coords> m_coords;
  std::shared_ptr<Masks> m_masks;
  std::shared_ptr<Attrs> m_attrs;
  bool m_readonly{false};
};

SCIPP_DATASET_EXPORT bool operator==(const DataArray &a, const DataArray &b);
SCIPP_DATASET_EXPORT bool operator!=(const DataArray &a, const DataArray &b);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
copy(const DataArray &array, AttrPolicy attrPolicy = AttrPolicy::Keep);

} // namespace scipp::dataset

namespace scipp {
using dataset::DataArray;
} // namespace scipp
