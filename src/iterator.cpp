#include <vector>
#include <memory>

// Use for non-in-place operations
class WorkspaceItem {
public:
  // How do we handle non-const access? Do we want references?
  // As long as this is const and returns by value we could make this work for a
  // HistogramView?!
  Histogram histogram() const;

  // All SpectrumInfo methods. Should probably always we const, otherwise we
  // cannot be thread-safe.
  const SpectrumDefinition &spectrumDefinition() const;
  const Eigen::Vector3d &position() const;
  bool isMasked()
      const; // Instrument masking, or obtained from link to MaskWorkspace??
};

// Use for in-place operations?
class MutableWorkspaceItem {
public:
  // Cannot work with HistogramView.
  // Is it confusing to have a variant returning by value, and this returning by
  // reference?
  Histogram &histogram();
  // Alternative, but causes copies:
  // const Histogram &histogram() const;
  // void setHistogram(Histogram histogram);

  // All SpectrumInfo methods. Should probably always we const, otherwise we
  // cannot be thread-safe.
  const SpectrumDefinition &spectrumDefinition() const;
  const Eigen::Vector3d &position() const;
  bool isMasked()
      const; // Instrument masking, or obtained from link to MaskWorkspace??
};

// How to support more abstract Histogram workspaces (with Histograms not linked
// to detector positions)?
template <class T> class HistogramWorkspace {
public:
  // Used by algorithms like Rebin that do not need metadata like positions for
  // the histograms.
  HistogramWorkspaceIterator histogramBegin();
  HistogramWorkspaceIterator histogramEnd();

  // Used by algorithms like ConvertUnits that need to access positions, Q, or
  // scattering angles for each detector.
  HistogramWorkspaceIterator<T> begin();
  HistogramWorkspaceIterator<T> end();

private:
  std::vector<Histogram> m_histograms;
  T m_spectrumInfo;
};

HistogramWorkspace<SpectrumInfo> detectorSpaceWorkspace;
HistogramWorkspace<QInfo> qSpaceWorkspace;
HistogramWorkspace<ScatteringAngleInfo> scatteringAngleWorkspace;

// How to support workspaces with a single data point per spectrum?
