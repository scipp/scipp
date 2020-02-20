// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "md_zip_view.h"

using namespace scipp::core;

// Quick and dirty conversion to strings, should probably be part of our library
// of basic routines.
std::vector<std::string> asStrings(const Variable &variable) {
  std::vector<std::string> strings;
  if (variable.dtype() == dtype<double>)
    for (const auto &value : variable.span<double>())
      strings.emplace_back(std::to_string(value));
  else if (variable.dtype() == dtype<std::string>)
    for (const auto &value : variable.span<std::string>())
      strings.emplace_back(value);
  return strings;
}

TEST(TableWorkspace, basics) {
  Dataset table;
  table.insert(Coord::Row, {Dim::Row, 3}, Vector<std::string>{"a", "b", "c"});
  table.insert(Data::Value, "", {Dim::Row, 3}, {1.0, -2.0, 3.0});
  table.insert(Data::DeprecatedString, "", {Dim::Row, 3}, 3);

  // Modify table with know columns.
  auto view =
      zipMD(table, MDRead(Data::Value), MDWrite(Data::DeprecatedString));
  for (auto &item : view)
    if (item.value() < 0.0)
      item.get(Data::DeprecatedString) = "why is this negative?";

  // Get string representation of arbitrary table, e.g., for visualization.
  EXPECT_EQ(asStrings(table(Coord::Row)),
            std::vector<std::string>({"a", "b", "c"}));
  EXPECT_EQ(asStrings(table(Data::Value, "")),
            std::vector<std::string>({"1.000000", "-2.000000", "3.000000"}));
  EXPECT_EQ(asStrings(table(Data::DeprecatedString, "")),
            std::vector<std::string>({"", "why is this negative?", ""}));

  // Standard shape operations provide basic things required for tables:
  auto mergedTable = concatenate(table, table, Dim::Row);
  Dataset row = table(Dim::Row, 1, 2);
  EXPECT_EQ(row.get(Coord::Row)[0], "b");

  // Slice a range to obtain a new table with a subset of rows.
  Dataset rows = mergedTable(Dim::Row, 1, 4);
  ASSERT_EQ(rows.get(Coord::Row).size(), 3);
  EXPECT_EQ(rows.get(Coord::Row)[0], "b");
  EXPECT_EQ(rows.get(Coord::Row)[1], "c");
  EXPECT_EQ(rows.get(Coord::Row)[2], "a");

  // Can sort by arbitrary column.
  auto sortedTable = sort(table, Data::Value);
  EXPECT_EQ(asStrings(sortedTable(Coord::Row)),
            std::vector<std::string>({"b", "a", "c"}));
  EXPECT_EQ(asStrings(sortedTable(Data::Value, "")),
            std::vector<std::string>({"-2.000000", "1.000000", "3.000000"}));
  EXPECT_EQ(asStrings(sortedTable(Data::DeprecatedString, "")),
            std::vector<std::string>({"why is this negative?", "", ""}));

  // Remove rows from the middle of a table.
  auto recombined = concatenate(mergedTable(Dim::Row, 0, 2),
                                mergedTable(Dim::Row, 4, 6), Dim::Row);
  EXPECT_EQ(asStrings(recombined(Coord::Row)),
            std::vector<std::string>({"a", "b", "b", "c"}));

  // Other basics (to be implemented): cut/truncate/chop/extract (naming
  // unclear), filter, etc.
}
