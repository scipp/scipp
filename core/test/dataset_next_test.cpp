// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "dataset_next.h"
#include "dimensions.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::core::next;

TEST(DatasetNext, construct_default) { ASSERT_NO_THROW(next::Dataset d); }

TEST(DatasetNext, empty) {
  next::Dataset d;
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.size(), 0);
}

TEST(DatasetNext, coords) {
  next::Dataset d;
  ASSERT_NO_THROW(d.coords());
}

TEST(DatasetNext, bad_item_access) {
  next::Dataset d;
  ASSERT_ANY_THROW(d[""]);
  ASSERT_ANY_THROW(d["abc"]);
}

TEST(DatasetNext, setCoord) {
  next::Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 0);

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 1);

  ASSERT_NO_THROW(d.setCoord(Dim::Y, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 2);

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 2);
}

TEST(DatasetNext, setValues) {
  next::Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_NO_THROW(d.setValues("a", var));
  ASSERT_EQ(d.size(), 1);

  ASSERT_NO_THROW(d.setValues("b", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setValues("a", var));
  ASSERT_EQ(d.size(), 2);
}

TEST(CoordsConstProxy, bad_item_access) {
  next::Dataset d;
  const auto coords = d.coords();
  ASSERT_ANY_THROW(coords[Dim::X]);
}

TEST(CoordsConstProxy, item_access) {
  next::Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {4, 5});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);

  const auto coords = d.coords();
  ASSERT_EQ(coords[Dim::X], x);
  ASSERT_EQ(coords[Dim::Y], y);
}

TEST(DataConstProxy, hasValues_hasVariances) {
  next::Dataset d;
  const auto var = makeVariable<double>({});

  d.setValues("a", var);
  d.setVariances("b", var);
  d.setValues("c", var);
  d.setVariances("c", var);

  ASSERT_TRUE(d["a"].hasValues());
  ASSERT_FALSE(d["a"].hasVariances());

  ASSERT_FALSE(d["b"].hasValues());
  ASSERT_TRUE(d["b"].hasVariances());

  ASSERT_TRUE(d["c"].hasValues());
  ASSERT_TRUE(d["c"].hasVariances());
}

#if 0
coords()[Dim::X]
coords().size()

coords(Dim::X)
coordsSize()


coords()[Dim::Position]
coords()["specNum"]

coords().size() // 2


coords()[0]
coords()[1]

coords().begin()
coords().end()

for dim, coord in dataset.coords:

for name, coord in dataset.aux_coords:


for key, coord in dataset.coords:
  if len(key.name) == 0:
    print('dim coord')

class Dataset {
std::vector<Variables> m_coords;

auto coords(const Dim dim) {
  for (const auto &coord : m_coords)
    if(coord.dimensions().inner() == dim)
      return ConstVariableSlice(coord);
  throw;
};
  

d.coords()

d.attrs()["specNum"]
d.labels()["specNum"]

d.coords()[Dim.Tof] // X axis
d.coords()[Dim.Position]
d["sample"].coords()[Dim.Position] // mapped from global coords
d["sample"].coords()[Dim.Tof] // event tof (replacing global coord)
#endif
