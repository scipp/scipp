#include <vector>
#include <memory>

#include <boost/tti/has_static_member_function.hpp>

template <class Data, class Instrument> struct Workspace {
  Data data; // can be std::vector<Histogram>, std::vector<DataPoint>, ...
  Instrument instrument; // can be SpectrumInfo, QInfo, ...
  std::vector<bool> *masking{nullptr};
};

using Histogram = std::vector<double>;
using DataPoint = std::pair<double, double>;
using SpectrumInfo = std::vector<float>;
using QInfo = std::vector<int>;

// Algorithms:
// - Scale
// - Rebin
// - ConvertUnits

template <class T> T scale(const T &data, const double scale) {
  return data * scale;
}

Histogram rebin(Histogram data) {
  data.resize(data + 1);
  return data;
}

// At the very least, different data type implies a different workspace type.
// All algorithms need to be templated?
// What about algorithms that do not touch the data, but just modify metadata?
// - Previously this is handled by using access via a base class.
// - This does not work anymore if there are multiple pieces to a workspace that
// can vary. For example, for an algorithm working with SpectrumInfo or QInfo,
// how can it avoid knowing the concrete workspace? Only works if nothing but
// the data is encoded in the type. This would imply that we need potentially
// empty pointers to metadata that is optional or variable.
// - The base class would only be useful for handling metadata, not the actual
// data, so there is maybe no point in inheritance?
template <class T> void transform(T &ws) {
  for (auto &item : ws)
    item = transform(item);
}

struct MetaData {
  Instrument instrument;
  Log logs;
  History history;
};

template <class Data> struct Workspace {
  Data data;
  MetaData metaData;
};

// Workspaces:
// Data: 10+ choices (but more resulting workspace types!?)
// Instrument: ~3 choices / optional
// Masking, Views: optional
// History, Logs: 1 choice
// Mapping from data items to instrument: optional (depends on Data)

// Algorithms:
// - some work only on data
// - some work only on non-data
// - many modify data and read non-data

// Workspace types:
// - Histograms
// - EventLists
// - Table
// - DataPoints (vector of value + error)
// - DataPoint (value + error)
// - Double
// - Int
// - HKLs/Peaks (several?)
// - Masking
// - Grouping
// - Instrument?

// In general, we want to point an algorithm at a workspace, and it should
// automatically:
// - pick the information it needs and do its work,
// - not require knowledge of the workspace from the algorithm implementer.
// Simple in Python? Workspace is dict, just unpack it into keyword arguments,
// ignore extras. Maybe not so simple, how can we deal with overloads?

// Pass supported workspace types as parameter pack, such that we do not call
// non-existing exec variants in Alg.
template <class Alg> void exec(Alg &alg, ADSHandle ws) {
  if (ws.type() == typeid(EventWorkspace))
    alg.exec(ws.cast<EventWorkspace>);
  // etc.
}

class Histograms {
public:
private:
  std::vector<Histogram> m_histograms;
};

template <class T> class Workspace {
public:
  const Histograms &histograms() const { return m_histograms; }
  void setHistograms(Histograms &&histograms) {
    m_histograms = std::move(histograms);
  }
  const T &metadata() const { return m_metadata; }

private:
  Histograms m_histograms;
  T m_metadata;
};

Histograms rebin(const Histograms &histograms) { return Histograms{}; }
Histograms convertUnits(const Histograms &histograms,
                        const SpectrumInfo &spectrumInfo) {
  return Histograms{};
}
Histograms convertUnits(const Histograms &histograms,
                        const IncidentWavelength &wavelength) {
  return Histograms{};
}

BOOST_TTI_HAS_STATIC_MEMBER_FUNCTION(exec);

class Algorithm {
public:
  // Enabled if T needs only histograms for processing
  // sizeof(U) is a workaround, because we must use U in the enable_if clause.
  template <class T, class U>
  Workspace<U>
  execute(const Workspace<U> &ws,
          typename std::enable_if<
              sizeof(U) &&
              has_static_member_function_exec<
                  T, Histograms,
                  boost::mpl::vector<const Histograms &>>::value>::type * =
              nullptr) {
    auto out(ws);
    out.setHistograms(T::exec(ws.histograms()));
    return out;
  }

  // Enabled if T needs histograms and metadata for processing
  template <class U>
  Workspace<U> execute(
      const Workspace<U> &ws,
      typename std::enable_if<has_static_member_function_exec<
          T, Histograms,
          boost::mpl::vector<const Histograms &, const U &>>::value>::type * =
          nullptr) {
    auto out(ws);
    out.setHistograms(T::exec(ws.histograms(), ws.metadata()));
    return out;
  }

  void execute(const boost::any &ws) {
    if (ws.type() == typeid(Workspace<SpectrumInfo>))
      execute(boost::any_cast < Workspace<SpectrumInfo>(ws));
    // other types
  }

  // Provide a fall back that does not all anything from Alg.
  template <class Alg, class... Comps>
  void execute(
      const Workspace<Comps...> &ws,
      typename std::enable_if<has_static_member_function_exec<
          Alg, Histograms,
          boost::mpl::vector<const Histograms &, const U &>>::value>::type * =
          nullptr) {}

  // TODO does this suffer from an explosion in the number of execute variants.
  // Are compile times ok?
  // TODO how to we get input workspace from property (instead of from
  // argument)? Lookup table based in type id in ADS? Combinatoric explosion
  // will kill us (compile times, binary size) if we have several input
  // workspaces. We actually *know* the types of supported input workspaces,
  // based on exec variants supported by algorithm, does that help us? Would a
  // LUT actually work? It would branch to non-existing overloads! Would need to
  // use a throwing base implementation?
};

struct Rebin {
  static Histograms exec(const Histograms &histograms) {
    return rebin(histograms);
  }
  // By providing exec for Histogram (or some other iterator-related helper
  // item) we can support stacking algorithm via Algorithm.
};

// Pass information contained in workspace as exec arguments, algorithm
// properties as constructor arguments?
struct ConvertUnits {
  // Set all properties except for workspaces?
  // Basically this implies that all algorithms need to deal with getting the
  // right properties.
  void setProperties(const std::vector<Property> &properties);
  // Could convert properties automatically in Algorithm based on expected
  // signature? Wouldn't it just do that automatically with our current property
  // system?
  // alg.setProperties(getProperty("TargetUnit"));
  void setProperties(const std::string &targetUnit);
  // Why separate setting properties from exec call with workspace?
  // - Might be helpful if we want to support stacks/groups?
  // - exec might have several overloads, dealing with various workspace types.
  //   Other properties should usually have fixed type?

  static Histograms exec(const Histograms &histograms,
                         const SpectrumInfo &spectrumInfo) {
    return convertUnits(histograms, spectrumInfo);
  }
  static Histograms exec(const Histograms &histograms,
                         const IncidentWavelength &wavelength) {
    return convertUnits(histograms, wavelength);
  }
};

int main() {
  Histograms hists;
  auto rebinnedHists = rebin(hists);

  Workspace<SpectrumInfo> ws;
  Workspace<IncidentWavelength> ws2;
  // We would like to call `rebin` on `ws`, which will obviously not work:
  // auto rebinnedWs = rebin(ws);

  // Wrapping in `Algorithm` does what we need:
  Algorithm<Rebin> alg;
  auto rebinnedWs = alg.execute(ws);
  auto rebinnedWs2 = alg.execute(ws2);

  // Composition can be handled via templated Algorithm::execute, actual
  // algorithms do not need templates. Note in particular that ws and ws2 are
  // *different types*, nevertheless `Rebin` works with both!
  Algorithm<ConvertUnits> alg2;
  auto convertedWs = alg2.execute(rebinnedWs);
  auto convertedWs2 = alg2.execute(rebinnedWs2);
}

// TODO
// - How should output workspace generation work in general?
// - How should we pass other arguments?
// - Iterators?
// - Instead of passing indivudual arguments, should we create a temporay
//   forwarding workspace of "minimal" type, containing only required
//   information?
// - How can we use properties instead of arguments and return values in
//   Algorithm::execute?
