class Histogram {
  private:
    const BinEdges &m_binEdges; // might be shared, can only be const (actually also better to avoid messing up data)
    Values &m_values;
    Errors &m_errors;
    gsl::index m_stride{1}; // TODO measure performance overhead of supporting stride
};

// Requirements:
// 1. Cheap to pass by value
// 2. Data should be held either directly or in Dataset?
//
// BinEdges always const!? (allows for sharing)
// Unit?!


// reference to dataset? pointer to dataset?
//

Dataset d1;
Dataset d2;
auto h1 = d1.histogram(i); // Dataset does not contain histograms, this returns by value, but h references d!? Copy constructor, assignment??
auto h2 = d2.histogram(i);
h1 *= 2.0; // scale Y and E in d1
h1 = h2; // compare X in d1 and d2, copy Y and E from d2 to d1
Histogram h;
h = h2; // not in any Dataset

// Same thing, but histograms stored in Dataset, in *addition* to X, Y, E?
Dataset d1;
Dataset d2;
auto &h1 = d1.histogram(i); // reference, h1 references data in d1
auto h2 = d1.histogram(i); // value, copy constructor extracts data, stored internally?
h1 = h2; // sets data in d1

class Histogram {
  public:
    // accessors to X, Y, E
  private:
    const gsl::span<BinEdge> &m_binEdges; // always const to support sharing?
    gsl::span<Value> &m_values;
    gsl::span<Error> &m_errors;
    std::unique_ptr<std::tuple<BinEdges, Values, Errors>> m_data{nullptr};
};
// handle unit in Histogram, not in Values, Errors (to get arround threading issues)? What about BinEdges? will lead to out-of-data units if data columns in dataset are modified!
// just check the unit? handle as other values? reference unit in dataset, propagate for stand-alone histograms
// - how to modify dataset including uit change?
// h1 *= h2; // unit is global to dataset cannot use on histogram level!
// => use d1 *= d2? // no! may not want to edit all columns
// d1.get<Values, Errors> *= d2.get<Values, Errors>; // full column, can handle units
