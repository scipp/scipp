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

TEST(ScaleDataPointWorkspace, moved) {
  Workspace<DataPoint> ws(2);
  const auto oldAddr = &ws[0];

  auto scaled = call<Scale>(std::move(ws), 1.5);

  ASSERT_EQ(ws.size(), 0);
  ASSERT_EQ(&scaled[0], oldAddr);
  ASSERT_EQ(scaled.size(), 2);
  ASSERT_EQ(scaled[0].value, 1.5);
  ASSERT_EQ(scaled[1].value, 1.5);
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

TEST(ScaleDataPointWorkspace, different_workspace_type) {
  Workspace<DataPoint, QInfo> ws(2);
  const auto oldAddr = &ws[0];

  ws = call<Scale>(std::move(ws), 1.5);

  ASSERT_EQ(&ws[0], oldAddr);
  ASSERT_EQ(ws.size(), 2);
  ASSERT_EQ(ws[0].value, 1.5);
  ASSERT_EQ(ws[1].value, 1.5);
}

TEST(ClearLogs, not_in_place) {
  Workspace<DataPoint> ws(2);
  ws = call<Scale>(std::move(ws), 1.5);

  auto out = call<ClearLogs>(ws);

  ASSERT_NE(&out[0], &ws[0]);
  ASSERT_EQ(out.size(), 2);
  ASSERT_EQ(out[0].value, 1.5);
  ASSERT_EQ(out[1].value, 1.5);
}

TEST(ClearLogs, in_place) {
  Workspace<DataPoint> ws(2);
  ws = call<Scale>(std::move(ws), 1.5);
  const auto oldAddr = &ws[0];

  auto out = call<ClearLogs>(std::move(ws));

  ASSERT_EQ(&out[0], oldAddr);
  ASSERT_EQ(out.size(), 2);
  ASSERT_EQ(out[0].value, 1.5);
  ASSERT_EQ(out[1].value, 1.5);
}

TEST(FilterByLogValue, not_in_place) {
  Workspace<EventList> ws(2);

  auto out = call<FilterByLogValue>(ws, "temp1", 274.0, 275.0);

  ASSERT_NE(&out[0], &ws[0]);
  ASSERT_EQ(out.size(), 2);
}

TEST(FilterByLogValue, in_place) {
  Workspace<EventList> ws(2);
  const auto oldAddr = &ws[0];

  auto out = call<FilterByLogValue>(std::move(ws), "temp1", 274.0, 275.0);

  ASSERT_EQ(&out[0], oldAddr);
  ASSERT_EQ(out.size(), 2);
}

TEST(Rebin, Histogram) {
  Workspace<Histogram> ws(2);

  auto binned = call<Rebin>(std::move(ws), BinEdges{});

  ASSERT_EQ(ws.size(), 2);
  ASSERT_EQ(binned.size(), 2);
  ASSERT_EQ(typeid(decltype(binned)::value_type), typeid(Histogram));
}

TEST(Rebin, EventList) {
  Workspace<EventList> eventWs(2);

  auto binned = call<Rebin>(eventWs, BinEdges{});

  ASSERT_EQ(binned.size(), 2);
  ASSERT_EQ(typeid(decltype(eventWs)::value_type), typeid(EventList));
  ASSERT_EQ(typeid(decltype(binned)::value_type), typeid(Histogram));
}

TEST(Rebin, rebin_subset_via_IndexSet) {
  Workspace<EventList> eventWs(3);

  auto binned = call<Rebin>(eventWs, IndexSet{0, 2}, BinEdges{});

  ASSERT_EQ(binned.size(), 2);
  ASSERT_EQ(typeid(decltype(eventWs)::value_type), typeid(EventList));
  ASSERT_EQ(typeid(decltype(binned)::value_type), typeid(Histogram));
}

TEST(Fit, full_workspace) {
  Workspace<Histogram> ws(3);

  auto fitResult = call<Fit>(ws, Fit::Function{}, Fit::Parameters{});

  ASSERT_EQ(fitResult.size(), 3);
  ASSERT_EQ(typeid(decltype(fitResult)::value_type), typeid(Fit::Result));
}

TEST(Fit, subset) {
  Workspace<Histogram> ws(3);

  auto fitResult =
      call<Fit>(ws, IndexSet{0}, Fit::Function{}, Fit::Parameters{});

  ASSERT_EQ(fitResult.size(), 1);
  ASSERT_EQ(typeid(decltype(fitResult)::value_type), typeid(Fit::Result));
}
