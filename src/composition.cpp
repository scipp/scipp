#include <vector>
#include <memory>

#include <boost/tti/has_static_member_function.hpp>

using Histogram = double;
using SpectrumInfo = std::vector<double>;
using IncidentWavelength = double;

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
  template <class T, class U>
  static Workspace<U> execute(
      const Workspace<U> &ws,
      typename std::enable_if<has_static_member_function_exec<
          T, Histograms,
          boost::mpl::vector<const Histograms &>>::value>::type * = nullptr) {
    auto out(ws);
    out.setHistograms(T::exec(ws.histograms()));
    return out;
  }

  // Enabled if T needs histograms and metadata for processing
  template <class T, class U>
  static Workspace<U> execute(
      const Workspace<U> &ws,
      typename std::enable_if<has_static_member_function_exec<
          T, Histograms,
          boost::mpl::vector<const Histograms &, const U &>>::value>::type * =
          nullptr) {
    auto out(ws);
    out.setHistograms(T::exec(ws.histograms(), ws.metadata()));
    return out;
  }

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
  // item) we can support stacking algorithms via Algorithm.
};

struct ConvertUnits {
  static Histograms exec(const Histograms &histograms,
                         const SpectrumInfo &spectrumInfo) {
    return convertUnits(histograms, spectrumInfo);
  }
  static Histograms exec(const Histograms &histograms,
                         const IncidentWavelength &wavelength) {
    return convertUnits(histograms, wavelength);
  }

  // Set all properties except for workspaces?
  // Basically this implies that all algorithms need to deal with getting the
  // right properties.
  // void setProperties(const std::vector<Property> &properties);
  // Could convert properties automatically in Algorithm based on expected
  // signature? Wouldn't it just do that automatically with our current property
  // system?
  // alg.setProperties(getProperty("TargetUnit"));
  // void setProperties(const std::string &targetUnit);
  // Why separate setting properties from exec call with workspace?
  // - Might be helpful if we want to support stacks/groups?
  // - exec might have several overloads, dealing with various workspace types.
  //   Other properties should usually have fixed type?
};

int main() {
  Histograms hists;
  auto rebinnedHists = rebin(hists);

  Workspace<SpectrumInfo> ws;
  Workspace<IncidentWavelength> ws2;
  // We would like to call `rebin` on `ws`, which will obviously not work:
  // auto rebinnedWs = rebin(ws);

  // Wrapping in `Algorithm::execute` does what we need:
  auto rebinnedWs = Algorithm::execute<Rebin>(ws);
  auto rebinnedWs2 = Algorithm::execute<Rebin>(ws2);

  // Composition can be handled via templated Algorithm::execute, actual
  // algorithms do not need templates. Note in particular that ws and ws2 are
  // *different types*, nevertheless `Rebin` works with both!
  // ConvertUnits can use SpectrumInfo (detector positions) or
  // IncidentWavelength for unit conversion:
  auto convertedWs = Algorithm::execute<ConvertUnits>(rebinnedWs);
  auto convertedWs2 = Algorithm::execute<ConvertUnits>(rebinnedWs2);

  // ConvertUnits cannot deal with std::string as metadata, does not compile:
  // Workspace<std::string> unsupported;
  // Algorithm::execute<ConvertUnits>(unsupported);
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
