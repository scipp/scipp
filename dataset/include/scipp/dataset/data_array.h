// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <string>

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
  /*
  explicit DataArray(const DataArray &view,
                     const AttrPolicy attrPolicy = AttrPolicy::Keep);
  */

  DataArray(const DataArray &other);
  DataArray(DataArray &&other) = default;

  DataArray(Variable data, Coords coords, Masks masks, Attrs attrs,
            const std::string &name);
  explicit DataArray(Variable data, typename Coords::holder_type coords = {},
                     typename Masks::holder_type masks = {},
                     typename Attrs::holder_type attrs = {},
                     const std::string &name = "");

  DataArray &operator=(const DataArray &other);
  DataArray &operator=(DataArray &&other) = default;

  explicit operator bool() const noexcept { return m_data.operator bool(); }

  const std::string &name() const { return m_name; }
  void setName(const std::string &name);

  const Coords &coords() const;
  Coords &coords();

  const Masks &masks() const;
  Masks &masks();

  const Attrs &attrs() const;
  Attrs &attrs();

  Dimensions dims() const { return m_data.dims(); }
  DType dtype() const { return m_data.dtype(); }
  units::Unit unit() const { return m_data.unit(); }

  void setUnit(const units::Unit unit) { m_data.setUnit(unit); }

  /// Return true if the data array contains data variances.
  bool hasVariances() const { return m_data.hasVariances(); }

  /// Return untyped const view for data (values and optional variances).
  const Variable &data() const { return m_data; }
  /// Return untyped view for data (values and optional variances).
  Variable data() { return m_data; }

  /// Return typed const view for data values.
  template <class T> auto values() const { return m_data.values<T>(); }
  /// Return typed view for data values.
  template <class T> auto values() { return m_data.values<T>(); }

  /// Return typed const view for data variances.
  template <class T> auto variances() const { return m_data.variances<T>(); }
  /// Return typed view for data variances.
  template <class T> auto variances() { return m_data.variances<T>(); }

  void rename(const Dim from, const Dim to);

  DataArray &operator+=(const DataArray &other);
  DataArray &operator-=(const DataArray &other);
  DataArray &operator*=(const DataArray &other);
  DataArray &operator/=(const DataArray &other);
  DataArray &operator+=(const Variable &other);
  DataArray &operator-=(const Variable &other);
  DataArray &operator*=(const Variable &other);
  DataArray &operator/=(const Variable &other);

  void setData(Variable data);

  DataArray slice(const Slice &s) const;

private:
  std::string m_name;
  Variable m_data;
  Coords m_coords;
  std::shared_ptr<Masks> m_masks;
  std::shared_ptr<Attrs> m_attrs;
};

SCIPP_DATASET_EXPORT bool operator==(const DataArray &a, const DataArray &b);
SCIPP_DATASET_EXPORT bool operator!=(const DataArray &a, const DataArray &b);

} // namespace scipp::dataset

namespace scipp {
using dataset::DataArray;
} // namespace scipp
