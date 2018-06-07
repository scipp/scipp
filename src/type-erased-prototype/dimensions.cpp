#include <algorithm>

#include "data_array.h"
#include "dimensions.h"

Dimensions::Dimensions() = default;
Dimensions::Dimensions(const Dimension label, const gsl::index size) {
  add(label, size);
}
Dimensions::Dimensions(const Dimensions &other) : m_dims(other.m_dims) {
  if (other.m_raggedDim)
    m_raggedDim = std::make_unique<DataArray>(*other.m_raggedDim);
}
Dimensions::Dimensions(Dimensions &&other) = default;
Dimensions::~Dimensions() = default;
Dimensions &Dimensions::operator=(const Dimensions &other) {
  auto copy(other);
  std::swap(*this, copy);
}
Dimensions &Dimensions::operator=(Dimensions &&other) = default;

gsl::index Dimensions::count() const { return m_dims.size(); }

gsl::index Dimensions::volume() const {
  gsl::index volume{1};
  gsl::index raggedCorrection{1};
  for (gsl::index dim = 0; dim < count(); ++dim) {
    if (isRagged(dim)) {
      const auto &raggedInfo = raggedSize(dim);
      const auto &dependentDimensions = raggedInfo.dimensions();
      gsl::index found{0};
      for (gsl::index dim2 = 0; dim2 < dependentDimensions.count(); ++dim2) {
        for (gsl::index dim3 = dim + 1; dim3 < count(); ++dim3) {
          if (dependentDimensions.label(dim2) == label(dim3)) {
            ++found;
            break;
          }
        }
      }
      if (found != dependentDimensions.count())
        throw std::runtime_error(
            "Ragged size information contains extra dimensions.");
      const auto &sizes = raggedInfo.get<const Variable::DimensionSize>();
      volume *= std::accumulate(sizes.begin(), sizes.end(), 0);
      raggedCorrection = dependentDimensions.volume();
    } else {
      volume *= size(dim);
    }
  }
  volume /= raggedCorrection;
  return volume;
}

bool Dimensions::isRagged(const gsl::index i) const {
  const auto size = m_dims.at(i).second;
  return size == -1;
}

Dimension Dimensions::label(const gsl::index i) const {
  return m_dims.at(i).first;
}

gsl::index Dimensions::size(const gsl::index i) const {
  const auto size = m_dims.at(i).second;
  if (size == -1)
    throw std::runtime_error(
        "Dimension is ragged, size() not available, use raggedSize().");
  return size;
}

const DataArray &Dimensions::raggedSize(const gsl::index i) const {
  if (!m_raggedDim)
    throw std::runtime_error("No such dimension.");
  if (m_dims.at(i).second != -1)
    throw std::runtime_error(
        "Dimension is not ragged, use size() instead of raggedSize().");
  return *m_raggedDim;
}

void Dimensions::add(const Dimension label, const gsl::index size) {
  // TODO check duplicate dimensions
  m_dims.emplace_back(label, size);
}

void Dimensions::add(const Dimension label, const DataArray &raggedSize) {
  // TODO check duplicate dimensions
  if (!raggedSize.valueTypeIs<Variable::DimensionSize>())
    throw std::runtime_error("DataArray with sizes information for ragged "
                             "dimension is of wrong type.");
  m_dims.emplace_back(label, -1);
  if (m_raggedDim)
    throw std::runtime_error("Only one dimension can be ragged.");
  m_raggedDim = std::make_unique<DataArray>(raggedSize);
}
