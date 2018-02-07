#include <vector>
#include <memory>

using Histogram = double;

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

private:
  Histograms m_histograms;
  T m_metadata;
};

Histograms rebin(const Histograms &histograms) { return Histograms{}; }

template <class T> class Algorithm {
public:
  template <class U> Workspace<U> execute(const Workspace<U> &ws) {
    auto out(ws);
    out.setHistograms(T::exec(ws.histograms()));
    return out;
  }
};

struct Rebin {
  static Histograms exec(const Histograms &histograms) {
    return rebin(histograms);
  }
};

int main() {
  Histograms hists;
  auto rebinnedHists = rebin(hists);

  Workspace<int> ws;
  // We would like to call `rebin` on `ws`, which will obviously not work:
  // auto rebinnedWs = rebin(ws);

  Algorithm<Rebin> alg;
  auto rebinnedWs = alg.execute(ws);
}
