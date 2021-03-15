// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/core/element_array.h"
#include "scipp/units/dim.h"
#include "scipp/units/unit.h"

#include "test_macros.h"

using namespace scipp;

class Variable {
private:
  Dimensions m_dims;
  scipp::index m_offset{0};
  units::Unit m_unit;
  element_array<double> m_values;
  bool is_slice() const noexcept {
    return m_offset != 0 || m_dims.volume() != m_values.size();
  }

public:
  Variable() = default;
  Variable(const Dimensions &dims, const units::Unit &unit,
           const element_array<double> &values)
      : m_dims(dims), m_unit(unit), m_values(values) {
    if (dims.volume() != values.size())
      throw std::runtime_error("Dims do not match size");
  }

  const auto &dims() const { return m_dims; }
  const auto &unit() const { return m_unit; }

  auto begin() { return m_values.begin() + m_offset; }
  auto end() { return m_values.begin() + m_offset + m_dims.volume(); }
  auto begin() const { return m_values.begin() + m_offset; }
  auto end() const { return m_values.begin() + m_offset + m_dims.volume(); }

  scipp::span<const double> values() const { return {begin(), end()}; }
  scipp::span<double> values() { return {begin(), end()}; }

  bool operator==(const Variable &other) const {
    return dims() == other.dims() && unit() == other.unit() &&
           std::equal(begin(), end(), other.begin(), other.end());
  }
  bool operator!=(const Variable &other) const { return !operator==(other); }

  Variable slice(const Dim dim, const scipp::index offset) const {
    Variable out(*this);
    auto out_dims = dims();
    out_dims.erase(dim);
    out.m_dims = out_dims;
    out.m_offset = offset;
    return out;
  }

  void setunit(const units::Unit &unit) {
    if (m_unit == unit)
      return;
    if (m_dims.volume() != m_values.size())
      throw std::runtime_error("Cannot set unit on slice");
    m_unit = unit;
  }

  Variable deepcopy() const {
    return is_slice() ? Variable{dims(), unit(), {begin(), end()}}
                      : Variable{dims(), unit(), m_values.deepcopy()};
  }
};

// coord must prevent length change (but switch to edges ok?)
// data array data or dataset item data must prevent length change
// coords of dataset item may no be added?
// masks and attrs of dataset item CAN be added
// ds['a'].coords['x'] = x # should fail
// ds['a'].attrs['x'] = x # should NOT fail
// ds['a'].masks['x'] = x # should NOT fail
//

class Coords {
public:
  Coords() = default;
  Coords(std::initializer_list<std::pair<const Dim, Variable>> items_)
      : items(std::move(items_)) {}
  Coords(std::unordered_map<Dim, Variable> items_) : items(std::move(items_)) {}
  const auto &operator[](const Dim dim) const { return items.at(dim); }
  void setitem(const Dim dim, Variable coord) { items[dim] = coord; }
  bool contains(const Dim dim) const { return items.count(dim) == 1; }
  auto begin() const { return items.begin(); }
  auto end() const { return items.end(); }
  auto begin() { return items.begin(); }
  auto end() { return items.end(); }

private:
  std::unordered_map<Dim, Variable> items;
};

class DataArray {
public:
  DataArray() = default;
  DataArray(Variable data, Coords coords)
      : m_data(std::move(data)), m_coords(std::move(coords)) {}
  const auto &data() const { return m_data; }
  const auto &coords() const { return m_coords; }
  auto &coords() { return m_coords; }

private:
  Variable m_data;
  Coords m_coords;
};

struct Dataset {
  const auto &operator[](const std::string &name) const {
    return items.at(name);
  }
  void setitem(const std::string &name, DataArray item) {
    for (auto &&[dim, coord] : item.coords())
      setcoord(dim, coord);
    items[name] = std::move(item);
  }
  // Note that we cannot set coords on
  void setcoord(const Dim dim, const Variable &coord) {
    if (coords.contains(dim) && coords[dim] != coord)
      throw std::runtime_error("Coords not aligned");
    coords.setitem(dim, coord);
    for (auto &existing : items)
      if (existing.second.data().dims().contains(dim))
        existing.second.coords().setitem(dim, coord);
  }

  Coords coords;
  std::unordered_map<std::string, DataArray> items;
};

Variable copy(const Variable &var) { return var.deepcopy(); }
DataArray copy(const DataArray &da) { return da; }
Dataset copy(const Dataset &ds) { return ds; }

class PrototypeTest : public ::testing::Test {
protected:
  Dimensions dimsX = Dimensions(Dim::X, 3);
};

TEST_F(PrototypeTest, variable) {
  const auto var = Variable(dimsX, units::m, {1, 2, 3});
  EXPECT_EQ(Variable(var).values().data(), var.values().data()); // shallow copy
  EXPECT_NE(copy(var).values().data(), var.values().data());     // deep copy
  auto shared = var;
  shared.values()[0] = 1.1;
  EXPECT_EQ(var.values()[0], 1.1);
}

TEST_F(PrototypeTest, variable_slice) {
  const auto var = Variable(dimsX, units::m, {1, 2, 3});
  auto slice = var.slice(Dim::X, 1);
  EXPECT_EQ(slice, Variable(Dimensions(), units::m, {2}));
  EXPECT_ANY_THROW(slice.setunit(units::s));
  slice.values()[0] = 1.1;
  EXPECT_EQ(var.values()[1], 1.1);
  EXPECT_EQ(copy(slice), slice);
}

TEST_F(PrototypeTest, data_array) {
  const auto var = Variable(dimsX, units::m, {1, 2, 3});
  auto da = DataArray(var, Coords{});
  EXPECT_EQ(da.data().values().data(), var.values().data()); // shallow copy
  da.coords().setitem(Dim::X, var);
  EXPECT_EQ(da.coords()[Dim::X].values().data(),
            var.values().data()); // shallow copy
  for (const auto da2 : {DataArray(da), copy(da)}) {
    // shallow copy of data and coords
    EXPECT_EQ(da2.data().values().data(), da.data().values().data());
    EXPECT_EQ(da2.coords()[Dim::X].values().data(),
              da.coords()[Dim::X].values().data());
  }
}

TEST_F(PrototypeTest, data_array_coord) {
  const auto var = Variable(dimsX, units::m, {1, 2, 3});
  auto da = DataArray(var, {{Dim::X, Variable(dimsX, units::m, {2, 4, 8})}});
  const auto coord = da.coords()[Dim::X];
  da = DataArray(var, Coords{});
  EXPECT_TRUE(equals(
      coord.values(),
      Variable(dimsX, units::m, {2, 4, 8}).values())); // coord is sole owner
}

TEST_F(PrototypeTest, dataset) {
  auto da1 = DataArray(Variable(dimsX, units::m, {1, 2, 3}),
                       {{Dim::X, Variable(dimsX, units::m, {1, 1, 1})}});
  auto da2 = DataArray(Variable(dimsX, units::m, {1, 2, 3}), {});
  auto ds = Dataset();
  ds.setitem("a", da1);
  for (const auto ds2 : {Dataset(ds), copy(ds)}) {
    // shallow copy of items and coords
    EXPECT_EQ(ds2["a"].data().values().data(), ds["a"].data().values().data());
    EXPECT_EQ(ds2.coords[Dim::X].values().data(),
              ds.coords[Dim::X].values().data());
  }
}
