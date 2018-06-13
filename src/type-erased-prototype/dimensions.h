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

  bool operator==(const Dimensions &other) const;

  bool isRagged() const;
  gsl::index count() const;
  gsl::index volume() const;

  bool contains(const Dimension label) const;
  bool contains(const Dimensions &other) const;
  bool isRagged(const gsl::index i) const;
  bool isRagged(const Dimension label) const;
  Dimension label(const gsl::index i) const;
  gsl::index size(const gsl::index i) const;
  gsl::index size(const Dimension label) const;
  gsl::index offset(const Dimension label) const;
  void resize(const Dimension label, const gsl::index size);
  void erase(const Dimension label);

  const DataArray &raggedSize(const gsl::index i) const;
  const DataArray &raggedSize(const Dimension label) const;
  void add(const Dimension label, const gsl::index size);
  void add(const Dimension label, const DataArray &raggedSize);

  auto begin() const { return m_dims.begin(); }
  auto end() const { return m_dims.end(); }

  gsl::index index(const Dimension label) const;

private:
  std::vector<std::pair<Dimension, gsl::index>> m_dims;
  // In a Dataset, multiple DataArrays will reference the same ragged size
  // DataArray. How can we support shape operations without breaking sharing?
  std::unique_ptr<DataArray> m_raggedDim;
};

Dimensions merge(const Dimensions &a, const Dimensions &b);

Dimensions concatenate(const Dimension dim, const Dimensions &dims1,
                       const Dimensions &dims2);

#endif // DIMENSIONS_H
