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
  Result operator()(const Histogram &histogram) const {}
};

// Category 2, option 1
struct Scale {
  static void apply(Histogram &histogam, const double factor) {}
  static void apply(DataPoint &dataPoint, const double factor) {}
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
  FilterByLogValue(const std::string &name, const double min,
                   const double max) {}
  void operator()(EventList &eventList) const {}
};

// Category 4, option 2
std::pair<int64_t, int64_t> makeFilterRangeByLogValue(const std::string &name,
                                                      const double min,
                                                      const double max) {
  return {0, 1};
}
void applyFilterRange(EventList &eventList,
                      const std::pair<int64_t, int64_t> &interval) {}
