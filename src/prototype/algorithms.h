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

// Category 2
struct Scale {
  static void apply(Histogram &histogam, const double factor) {}
  static void apply(DataPoint &dataPoint, const double factor) {
    dataPoint.value *= factor;
  }
};

struct Rebin {
  Histogram apply(const Histogram &histogram, const BinEdges &binEdges) const {
    return histogram;
  }
  Histogram apply(const EventList &eventList, const BinEdges &binEdges) const {
    return {};
  }
};

// Category 3
struct ClearLogs {
  static void apply(Logs &logs) {}
};

// Category 4
class FilterByLogValue {
public:
  FilterByLogValue(const Logs &logs, const std::string &name, const double min,
                   const double max) {}
  void apply(EventList &eventList) const {}
};

// Category 5
struct ConvertUnit {
  class Unit {};
  template <class T>
  class Spectrum {};
  // How do we set the unit on the target workspace?
  ConvertUnit(const Unit &target) {}
  // Spectrum contains read-only links to instrument?
  void apply(Spectrum<Histogram> &histogram) const {}
  void apply(Spectrum<EventList> &eventList) const {}
  // One option is a separate apply. Is it possible to call more than one in the
  // call wrapper? How do we define a proper ordering (is that required?)?
  // As defined currently, this would enable multiple Apply definitions. Should
  // we just give it another name, such as preApply() and postApply()?
  void apply(Unit &unit) const {}
};

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
