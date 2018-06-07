#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#include <memory>
#include <utility>
#include <vector>

#include "dimension.h"
#include "index.h"

class DataArray;

class Dimensions {
public:
  Dimensions();
  Dimensions(const Dimension label, const gsl::index size);
  Dimensions(const Dimensions &other);
  Dimensions(Dimensions &&other);
  ~Dimensions();
  Dimensions &operator=(const Dimensions &other);
  Dimensions &operator=(Dimensions &&other);

  gsl::index count() const;
  gsl::index volume() const;

  bool isRagged(const gsl::index i) const;
  Dimension label(const gsl::index i) const;
  gsl::index size(const gsl::index i) const;

  const DataArray &raggedSize(const gsl::index i) const;
  void add(const Dimension label, const gsl::index size);
  void add(const Dimension label, const DataArray &raggedSize);

private:
  std::vector<std::pair<Dimension, gsl::index>> m_dims;
  std::unique_ptr<DataArray> m_raggedDim;
};

#endif // DIMENSIONS_H
