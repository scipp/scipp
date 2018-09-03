/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset_view.h"

// Quick and dirty conversion to strings, should probably be part of our library
// of basic routines.
std::vector<std::string> asStrings(const Variable &variable) {
  std::vector<std::string> strings;
  if (variable.valueTypeIs<Coord::RowLabel>())
    for (const auto &value : variable.get<Coord::RowLabel>())
      strings.emplace_back(value);
  if (variable.valueTypeIs<Data::Value>())
    for (const auto &value : variable.get<Data::Value>())
      strings.emplace_back(std::to_string(value));
  else if (variable.valueTypeIs<Data::String>())
    for (const auto &value : variable.get<Data::String>())
      strings.emplace_back(value);
  return strings;
}

TEST(TableWorkspace, basics) {
  Dataset table;
  table.insert<Coord::RowLabel>({Dimension::Row, 3},
                                Vector<std::string>{"a", "b", "c"});
  table.insert<Data::Value>("Data", {Dimension::Row, 3}, {1.0, -2.0, 3.0});
  table.insert<Data::String>("Comment", {Dimension::Row, 3}, 3);

  // Modify table with know columns.
  DatasetView<const Data::Value, Data::String> view(table);
  for (auto &item : view)
    if (item.value() < 0.0)
      item.get<Data::String>() = "why is this negative?";

  // Get string representation of arbitrary table, e.g., for visualization.
  EXPECT_EQ(asStrings(table[0]), std::vector<std::string>({"a", "b", "c"}));
  EXPECT_EQ(asStrings(table[1]),
            std::vector<std::string>({"1.000000", "-2.000000", "3.000000"}));
  EXPECT_EQ(asStrings(table[2]),
            std::vector<std::string>({"", "why is this negative?", ""}));

  // Standard shape operations provide basic things required for tables:
  auto mergedTable = concatenate(Dimension::Row, table, table);
  auto row = slice(table, Dimension::Row, 1);
  EXPECT_EQ(row.get<const Coord::RowLabel>()[0], "b");
  // Other basics (to be implemented): cut/truncate/chop/extract (naming
  // unclear), sort, filter, etc.
}
