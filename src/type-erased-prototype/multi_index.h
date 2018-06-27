#ifndef MULTI_INDEX_H
#define MULTI_INDEX_H

#include "dimensions.h"

class MultiIndex {
public:
  MultiIndex(const Dimensions &parentDimensions,
             const std::vector<Dimensions> &subdimensions) {
    if (parentDimensions.count() > 4)
      throw std::runtime_error("MultiIndex supports at most 4 dimensions.");
    if (subdimensions.size() > 4)
      throw std::runtime_error("MultiIndex supports at most 4 subindices.");
    m_dims = parentDimensions.count();
    for (gsl::index d = 0; d < m_dims; ++d)
      m_extent[d] = parentDimensions.size(d);

    for (const auto &dimensions : subdimensions) {
      gsl::index factor{1};
      std::vector<gsl::index> offsets;
      std::vector<gsl::index> factors;
      for (gsl::index i = 0; i < dimensions.count(); ++i) {
        const auto dimension = dimensions.label(i);
        if (parentDimensions.contains(dimension)) {
          offsets.push_back(parentDimensions.index(dimension));
          factors.push_back(factor);
        }
        factor *= dimensions.size(i);
      }
      m_offsets.push_back(offsets);
      m_factors.push_back(factors);
    }
    gsl::index offset{1};
    for (gsl::index d = 0; d < m_dims; ++d) {
      setIndex(offset);
      m_delta[4 * d + 0] = get<0>();
      m_delta[4 * d + 1] = get<1>();
      m_delta[4 * d + 2] = get<2>();
      m_delta[4 * d + 3] = get<3>();
      if (d > 0) {
        setIndex(offset - 1);
        m_delta[4 * d + 0] -= get<0>() + m_delta[4 * (d - 1) + 0];
        m_delta[4 * d + 1] -= get<1>() + m_delta[4 * (d - 1) + 1];
        m_delta[4 * d + 2] -= get<2>() + m_delta[4 * (d - 1) + 2];
        m_delta[4 * d + 3] -= get<3>() + m_delta[4 * (d - 1) + 3];
      }
      offset *= m_extent[d];
    }
    setIndex(0);
  }

  void increment() {
    // gcc does not vectorize the addition for some reason.
    for (int i = 0; i < 4; ++i)
      m_index[i] += m_delta[0 + i];
    ++m_coord[0];
    if (m_coord[0] == m_extent[0]) {
      m_coord[0] = 0;
      for (int i = 0; i < 4; ++i)
        m_index[i] += m_delta[4 + i];
      ++m_coord[1];
      if (m_dims > 1 && m_coord[1] == m_extent[1]) {
        m_coord[1] = 0;
        for (int i = 0; i < 4; ++i)
          m_index[i] += m_delta[8 + i];
        ++m_coord[2];
        if (m_dims > 2 && m_coord[2] == m_extent[2]) {
          m_coord[2] = 0;
          for (int i = 0; i < 4; ++i)
            m_index[i] += m_delta[12 + i];
          ++m_coord[3];
        }
      }
    }
  }

  void setIndex(const gsl::index index) {
    if (m_dims == 0)
      return;
    auto remainder{index};
    for (gsl::index d = 0; d < m_dims - 1; ++d) {
      m_coord[d] = remainder % m_extent[d];
      remainder /= m_extent[d];
    }
    m_coord[m_dims - 1] = remainder;
    for (gsl::index i = 0; i < m_factors.size(); ++i) {
      m_index[i] = 0;
      for (gsl::index j = 0; j < m_factors[i].size(); ++j)
        m_index[i] += m_factors[i][j] * m_coord[m_offsets[i][j]];
    }
  }

  template <int N> gsl::index get() const { return m_index[N]; }

  // TODO Always use full dimensions for index 0. Or compare m_coord instead?
  bool operator==(const MultiIndex &other) {
    return m_index[0] == other.m_index[0];
  }

private:
  // alignas does not help, for some reason gcc does not generate SIMD
  // instructions.
  // Using std::array is 1.5x slower, for some reason intermediate values of
  // m_index are always stored instead of merely being kept in regsiters.
  alignas(32) gsl::index m_index[4];
  alignas(32) gsl::index m_delta[16];
  alignas(32) gsl::index m_coord[4];
  alignas(32) gsl::index m_extent[4];
  gsl::index m_dims;
  std::vector<std::vector<gsl::index>> m_offsets;
  std::vector<std::vector<gsl::index>> m_factors;
};

#endif // MULTI_INDEX_H
