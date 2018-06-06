#include "data_array.h"
#include "dimensions.h"

Dimensions::Dimensions() = default;
Dimensions::Dimensions(const Dimensions &other)
    : m_dims(other.m_dims),
      m_raggedDim(std::make_unique<DataArray>(*other.m_raggedDim)) {}
Dimensions::Dimensions(Dimensions &&other) = default;
Dimensions &Dimensions::operator=(const Dimensions &other) {
  auto copy(other);
  std::swap(*this, copy);
}
Dimensions &Dimensions::operator=(Dimensions &&other) = default;

gsl::index Dimensions::count() const { return m_dims.size(); }

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
