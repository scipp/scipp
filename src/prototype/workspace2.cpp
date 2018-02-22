
// Grouping?
// What about time index for scanning?
// This does not work:
Workspace<std::pair<SpectrumNumber, TimeIndex>>;
// Tightly coupled into SpectrumInfo, should it just be part of that?
// how do we setup grouping?
auto grouping = call<Load>("grouping.nxs"); // contain instrument?
// if it would not link to the instrument it cannot be validated -> pay validation cost with every workspace creation
Workspace<Histogram> ws(grouping);
// or
auto ws = call<CreateWorkspace>(grouping); // copy/share grouping internals, or actual workspace?
// Question: Who needs to access the grouping?
// - SpectrumInfo (etc.)
// - algorithms changing the grouping?

// Bin masking
// should mask flag be a float/double? What happens on rebin?
// Bin edges should be same as linked workspace, how do we enforce this without having BinEdges in this workspace as well? Should mask just be optional in Histogram?
using BinMaskWorkspace = Workspace<std::vector<bool>>;
using BinMaskWorkspace = Workspace<std::vector<float>>;
// should bin masking also handle full masks of spectrum?
// should masking be the same as selections (IndexSet?)?

class Workspace {
public:
  Counts counts() const { return m_counts; }
  // here i is the outermost index, indexing the instrument
  // can we be more verbose?
  DataPoint operator[](const size_t i) const {
    return DataPoint(m_counts[i], m_errors[i]);
  }

private:
  Counts m_counts;
  CountStandardDeviations m_errors;
  double m_x;
  std::vector<SpectrumNumber> m_spectrumNumbers;
};

class Workspace {
public:
  DataPoint dataPoint(const size_t index, const size_t x) const {
    // Note the order, index, which maps to the instrument, is the *inner* index.
    return DataPoint(m_counts[x][index], m_errors[x][index]);
  }

  // x would always be 0 for fixed-incident-wavelength instrumentsgg
  Counts counts(const size_t x) const {
    return m_counts[x];
  }
  // how can we avoid rewriting algorithms?
  // - write things working with data only in terms of Counts/Errors (not Histogram?)?
  // - provide overloads for DataPoint (inefficient)
  // - do we need a way to provide information from instrument as "axis"?
  // - is this almost a MD workspace, if we have QInfo instead of SpectrumInfo?
  // - can this concept be extended to support parameters scans?

private:
  std::vector<Counts> m_counts;
  std::vector<CountStandardDeviations> m_errors;
  Points m_points; // same length as m_counts, NOT as Counts!
                   // or: BinEdges m_binEdges;
  std::vector<SpectrumNumber> m_spectrumNumbers;
};

class Workspace {
public:
  Histogram histogram(const size_t i) const {
    return Histogram(m_points[i], m_counts[i], m_errors[i]);
  }

private:
  std::vector<Counts> m_counts;
  std::vector<CountStandardDeviations> m_errors;
  std::vector<Points> m_points;
  std::vector<SpectrumNumber> m_spectrumNumbers;
}


// Inner components, some of them optional:
std::tuple<Counts, CountStdDevs, Mask, DeltaQ>;

// For normal TOF data:
std::pair<BinEdges, std::tuple<Counts, CountStdDevs, Mask, DeltaQ>>;

// Basic workspace:
std::vector<std::pair<BinEdges, std::tuple<Counts, CountStdDevs, Mask, DeltaQ>>>;


template <class Index, class... Data>
class Series {
  public:
    Series(Index index, Data... data)
        : m_index(std::move(index)),
          m_data(std::make_tuple(std::move(data...))) {}

    // begin(), end(), iterator for subset of data?
    // lookup via index?? via external helper that builds unordered_map?

  private:
    std::string m_name; // for std::vector<Series> behaving like a table? for TextAxis?
    Index m_index;
    std::tuple<Data...> m_data;
  // Index can be:
  // - BinEdges/Counts for "Histogram"
  // - Instrument/SpectrumInfo for constant-wavelength workspace (or slice of imaging workspace), or EventWorkspace (without bin Edges)?
  // - (multi-dimensional)QInfo for MDWorkspace
  //
  // Data can be:
  // - Counts, CountStdDevs, Mask, DeltaQ, ...
  // - if there are multiple data items, all are the same length
  //
  // Metadata (non-indexed?)
};

using Histogram = Series<BinEdges, Counts, CountStdDevs>;

template <class MetaData>
using Workspace2D = std::pair<Series<SpectrumInfo, std::vector<Histogram>>, MetaData>;

// strings are row labels
// Note: can directly have other objects like Counts as a column!
// Would want names for *columns*?! Should a Series have a name? Provide it as MetaData?
using TableWorkspace = Series<std::vector<std::string>, Col1Type, Col2Type, ...>

// Same as series??
template <class Index, class SeriesIndex, class... Data>
class SeriesVector {
  std::vector<Series<SeriesIndex, Data...>> m_data;
  Index m_index;
}

template <class Data, class MetaData>
class Workspace {
  Data m_data;
  MetaData m_metaData;
};

// Components of series should be accessible via standard interface. How? Just
// support all common options by hand? Mixins?
