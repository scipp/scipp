#include <boost/tti/has_function.hpp>

#include "algorithms.h"
#include "call_wrappers.h"
#include "workspace.h"

int main() {
  // Transform workspace, keeping type (an just copy to output and modify).
  Workspace<Histogram> ws;
  Workspace<Histogram, QInfo> qWs;
  ws = call<Scale>(std::move(ws), 2.3);
  qWs = call<Scale>(std::move(qWs), 2.3);
  ws = call<ClearLogs>(std::move(ws));
  Workspace<EventList> eventWs(5);
  eventWs = call<FilterByLogValue>(std::move(eventWs), "temp1", 274.0, 275.0);
  // Could use auto, this is just to make sure that we are getting the expected
  // type.
  Workspace<Histogram> binned = call<Rebin>(eventWs, BinEdges{});

  auto fitResult = call<Fit>(binned, Fit::Function{}, Fit::Parameters{});
  // Should IndexSet simply be a mask workspace?
  fitResult =
      call<Fit>(binned, IndexSet{2, 3}, Fit::Function{}, Fit::Parameters{});
}
