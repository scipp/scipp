#include <vector>
#include <memory>

// Part 1 Views

// Support Workspace2D, EventWorkspace, transpose, masking in child classes?
// Should this be called Model or View?
class HistogramView {
public:
  virtual ~HistogramView() = default;
  virtual Histogram operator[](const size_t i) = 0;
};

// Use this to get rid of MRU in EventWorkspace.
// Algorithms like ConvertUnits and Rebin need to support views!
class EventWorkspaceHistogramView : public HistogramView {
public:
  EventWorkspaceHistogramView(const EventWorkspace data) : m_data(data) {}
  Histogram operator[](const size_t i)override {
    return rebin(m_data[i], m_binEdges[i]);
  }

private:
  const EventWorkspace &m_data;
  std::vector<BinEdges> m_binEdges;
};

// PanelView -> this is what ILL (SANS?) asked for, but isn't this simply what
// the instrument view provides?

// Part 2 Links / workspace-type vs. workspace-composition redundancy

class GroupingWorkspace {
public:
  const SpectrumDefinition &spectrumDefinition(const size_t i) const {
    return m_spectrumDefinitions[i];
  }

private:
  std::vector<SpectrumDefinition> m_spectrumDefinitions;
};

class Workspace2D {
public:
private:
  std::vector<Histogram> m_histograms;
  const GroupingWorkspace &m_grouping; // Do not store grouping anywhere else
                                       // (ISpectrum, IndexInfo, SpectrumInfo)
};

int main() {}
