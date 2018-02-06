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
