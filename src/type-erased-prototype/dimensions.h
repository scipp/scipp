/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#include <memory>
#include <utility>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <gsl/gsl_util>

#include "dimension.h"

class Variable;

class Dimensions {
public:
  Dimensions();
  Dimensions(const Dimension label, const gsl::index size);
  Dimensions(const std::vector<std::pair<Dimension, gsl::index>> &sizes);
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

  const Variable &raggedSize(const gsl::index i) const;
  const Variable &raggedSize(const Dimension label) const;
  void add(const Dimension label, const gsl::index size);
  void add(const Dimension label, const Variable &raggedSize);

  auto begin() const { return m_dims.begin(); }
  auto end() const { return m_dims.end(); }

  gsl::index index(const Dimension label) const;

private:
  boost::container::small_vector<std::pair<Dimension, gsl::index>, 2> m_dims;
  // In a Dataset, multiple Variables will reference the same ragged size
  // Variable. How can we support shape operations without breaking sharing?
  std::unique_ptr<Variable> m_raggedDim;
};

Dimensions merge(const Dimensions &a, const Dimensions &b);

Dimensions concatenate(const Dimension dim, const Dimensions &dims1,
                       const Dimensions &dims2);

#endif // DIMENSIONS_H
