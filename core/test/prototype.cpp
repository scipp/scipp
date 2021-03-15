// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <vector>

#include "scipp/core/dimensions.h"
#include "scipp/core/element_array.h"
#include "scipp/units/dim.h"

using namespace scipp;

struct Variable {
  Variable() = default;
  Variable(const Dimensions &dims_, const std::vector<double> &values_)
      : dims(dims_), values(values_) {}
  bool operator==(const Variable &other) const {
    return dims == other.dims && values == other.values;
  }
  bool operator!=(const Variable &other) const {
    return dims != other.dims || values != other.values;
  }
  Dimensions dims;
  element_array<double> values;
};

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

struct DataArray {
  DataArray() = default;
  DataArray(Variable data_, Coords coords_)
      : data(std::move(data_)), coords(std::move(coords_)) {}
  Variable data;
  Coords coords;
};

struct Dataset {
  const auto &operator[](const std::string &name) const {
    return items.at(name);
  }
  void setitem(const std::string &name, DataArray item) {
    for (auto &&[dim, coord] : item.coords)
      setcoord(dim, std::forward<Variable>(coord));
    items[name] = std::move(item);
  }
  // Note that we cannot set coords on
  void setcoord(const Dim dim, const Variable &coord) {
    if (coords.contains(dim) && coords[dim] != coord)
      throw std::runtime_error("Coords not aligned");
    coords.setitem(dim, coord);
    for (auto &existing : items)
      if (existing.second.data.dims.contains(dim))
        existing.second.coords.setitem(dim, coord);
  }

  Coords coords;
  std::unordered_map<std::string, DataArray> items;
};

Variable
copy(const Variable &var) {
  auto out(var);
  out.values = out.values.deepcopy();
  return out;
}

DataArray copy(const DataArray &da) { return da; }
Dataset copy(const Dataset &ds) { return ds; }

class PrototypeTest : public ::testing::Test {
protected:
  Dimensions dimsX = Dimensions(Dim::X, 3);
};

TEST_F(PrototypeTest, variable) {
  const auto var = Variable(dimsX, {1, 2, 3});
  EXPECT_EQ(Variable(var).values.data(), var.values.data()); // shallow copy
  EXPECT_NE(copy(var).values.data(), var.values.data());     // deep copy
}

TEST_F(PrototypeTest, data_array) {
  const auto var = Variable(dimsX, {1, 2, 3});
  auto da = DataArray(var, Coords{});
  EXPECT_EQ(da.data.values.data(), var.values.data()); // shallow copy
  da.coords.setitem(Dim::X, var);
  EXPECT_EQ(da.coords[Dim::X].values.data(),
            var.values.data()); // shallow copy
  for (const auto da2 : {DataArray(da), copy(da)}) {
    // shallow copy of data and coords
    EXPECT_EQ(da2.data.values.data(), da.data.values.data());
    EXPECT_EQ(da2.coords[Dim::X].values.data(),
              da.coords[Dim::X].values.data());
  }
}

TEST_F(PrototypeTest, data_array_coord) {
  const auto var = Variable(dimsX, {1, 2, 3});
  auto da = DataArray(var, {{Dim::X, Variable(dimsX, {2, 4, 8})}});
  const auto coord = da.coords[Dim::X];
  da = DataArray(var, Coords{});
  EXPECT_EQ(coord.values,
            Variable(dimsX, {2, 4, 8}).values); // coord is sole owner
}

TEST_F(PrototypeTest, dataset) {
  auto da1 = DataArray(Variable(dimsX, {1, 2, 3}),
                       {{Dim::X, Variable(dimsX, {1, 1, 1})}});
  auto da2 = DataArray(Variable(dimsX, {1, 2, 3}), {});
  auto ds = Dataset();
  ds.setitem("a", da1);
  for (const auto ds2 : {Dataset(ds), copy(ds)}) {
    // shallow copy of items and coords
    EXPECT_EQ(ds2["a"].data.values.data(), ds["a"].data.values.data());
    EXPECT_EQ(ds2.coords[Dim::X].values.data(),
              ds.coords[Dim::X].values.data());
  }
}
