#include <cstdint>
#include <string>
#include <vector>

#include "data.h"
#include "metadata.h"

// Category 1
class Fit {
public:
  class Function {};
  class Parameters {};
  class Result {};
  Fit(const Function &function, const Parameters &parameters) {}
  Result apply(const Histogram &histogram) const {}
};

// Category 2, option 1
struct Scale {
  static void apply(Histogram &histogam, const double factor) {}
  static void apply(DataPoint &dataPoint, const double factor) {}
};

struct Rebin {
  Histogram apply(const Histogram &histogram, const BinEdges &binEdges) const {
    return histogram;
  }
  Histogram apply(const EventList &eventList, const BinEdges &binEdges) const {
    return {};
  }
};

// Category 2, option 2
void scale(Histogram &histogam, const double factor) {}
void scale(DataPoint &dataPoint, const double factor) {}
Histogram rebin(const Histogram &histogram, const BinEdges &binEdges) {
  return histogram;
}
void sortEvents(EventList &eventList) {}

// Category 3
struct ClearLogs {
  static void apply(Logs &logs) {}
};

// Category 4, option 1
class FilterByLogValue {
public:
  FilterByLogValue(const Logs &logs, const std::string &name, const double min,
                   const double max) {}
  void apply(EventList &eventList) const {}
};

// Category 4, option 2
std::pair<int64_t, int64_t> makeFilterRangeByLogValue(const std::string &name,
                                                      const double min,
                                                      const double max) {
  return {0, 1};
}
void applyFilterRange(EventList &eventList,
                      const std::pair<int64_t, int64_t> &interval) {}

// Category X
// how would we handle verification, such as checking instrument compatibility?
struct Add {
  Histogram apply(const Histogram &a, const Histogram &b) { return a; }
};

// TODO
// - check that we are not accidentially copying inputs.
// - return non-workspace types? (example?)
// - multiple ws arguments (Add)
// - instrument (or other indexed) input (ConvertUnits)
// - selecting input indices, such as for Fit or ExtractSpectra?
// - modify multiple args (ConvertUnits?)
//
// - input validation / properties?
// - multi-part algorithms (transforming multiple components)?
// - algorithms working with actual workspace types
