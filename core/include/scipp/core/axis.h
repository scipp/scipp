// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_AXIS_H
#define SCIPP_CORE_AXIS_H

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/variable.h"

namespace scipp::core {

// d.coords
// d['a'].coords # align, thus the same
// d['a'].unaligned.coords
//
// d.masks
// d['a'].masks # would like to support different
// d['a'].unaligned.masks
//
// d.attrs
// d['a'].attrs
// d['a'].unaligned.attrs
class Axis : DataInterface<Axis> {
public:
  explicit Axis(Variable data) : m_data(std::move(data)) {}

  Axis &operator+=(const AxisConstView &other);
  Axis &operator-=(const AxisConstView &other);
  Axis &operator*=(const AxisConstView &other);
  Axis &operator/=(const AxisConstView &other);

private:
  Variable m_data;
  std::unordered_map<std::string, Variable> m_unaligned;
};

class AxisConstView : DataConstInterface<AxisConstView> {
public:
  AxisConstView(const Axis &axis)
      : m_data(axis.data),
        m_unaligned(axis.unaligned().begin(), axis.unaligned().end()) {}
  // Implicit conversion from VariableConstView useful for operators.
  AxisConstView(const VariableConstView &data) : m_data(data) {}

  // Need two different interfaces:
  // For coord of dataset
  const UnalignedConstView unaligned() const noexcept { return m_unaligned; }
  const UnalignedView unaligned() noexcept { return m_unaligned; }
  // For coord of data array or data of dataset item or data array
  const VarialeConstView unaligned() const noexcept { return m_unaligned; }
  const VarialeView unaligned() noexcept { return m_unaligned; }

private:
  VariableConstView m_data;
  std::unordered_map<std::string, VariableConstView> m_unaligned;
};

class AxisView : DataViewInterface<AxisView> {
public:
  AxisView operator+=(const AxisConstView &other) const;
  AxisView operator-=(const AxisConstView &other) const;
  AxisView operator*=(const AxisConstView &other) const;
  AxisView operator/=(const AxisConstView &other) const;

private:
  VariableView m_data;
  std::unordered_map<std::string, VariableView> m_unaligned;
};

DataArrayView slice(const Slice slice1) const;

DataArrayView assign(const DataArrayConstView &other) const;
DataArrayView assign(const Variable &other) const;
DataArrayView assign(const VariableConstView &other) const;


} // namespace scipp::core
#endif // SCIPP_CORE_AXIS_H
