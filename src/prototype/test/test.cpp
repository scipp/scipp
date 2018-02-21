#include <gtest/gtest.h>

#include "algorithms.h"
#include "call_wrappers.h"
#include "workspace.h"

TEST(DataPoint, initial_value) {
  DataPoint p;
  ASSERT_EQ(p.value, 1.0);
}

TEST(Workspace, size) {
  Workspace<DataPoint> ws(2);
  ASSERT_EQ(ws.size(), 2);
}

TEST(ScaleDataPointWorkspace, not_in_place) {
  Workspace<DataPoint> ws(2);

  auto scaled = call<Scale>(ws, 1.5);

  ASSERT_EQ(scaled.size(), 2);
  ASSERT_EQ(ws[0].value, 1.0);
  ASSERT_EQ(ws[1].value, 1.0);
  ASSERT_EQ(scaled[0].value, 1.5);
  ASSERT_EQ(scaled[1].value, 1.5);
}

TEST(ScaleDataPointWorkspace, replace_not_in_place) {
  Workspace<DataPoint> ws(2);
  const auto oldAddr = &ws[0];

  ws = call<Scale>(ws, 1.5);

  ASSERT_NE(&ws[0], oldAddr);
  ASSERT_EQ(ws.size(), 2);
  ASSERT_EQ(ws[0].value, 1.5);
  ASSERT_EQ(ws[1].value, 1.5);
}

TEST(ScaleDataPointWorkspace, in_place) {
  Workspace<DataPoint> ws(2);
  const auto oldAddr = &ws[0];

  ws = call<Scale>(std::move(ws), 1.5);

  ASSERT_EQ(&ws[0], oldAddr);
  ASSERT_EQ(ws.size(), 2);
  ASSERT_EQ(ws[0].value, 1.5);
  ASSERT_EQ(ws[1].value, 1.5);
}
