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

template <class T> class Algorithm {
public:
  // Enabled if T needs only histograms for processing
  // sizeof(U) is a workaround, because we must use U in the enable_if clause.
  template <class U>
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
};

struct Rebin {
  static Histograms exec(const Histograms &histograms) {
    return rebin(histograms);
  }
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
