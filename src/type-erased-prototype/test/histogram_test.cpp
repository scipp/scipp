#include <gtest/gtest.h>

#include "dataset.h"

class Histogram {
public:
  // should only be usable by Dataset (or factory)!
  Histogram(Dataset &d, const gsl::index i)
      : m_size(d.get<double>().size()),
        m_binEdges(d.get<double>().data()), // X always shared in this example
        m_values(d.get<int>().data() + i * m_size) {}
  Histogram(const Histogram &other) : m_size(other.m_size), m_binEdges(other.m_binEdges) {
    m_data = std::make_unique<std::vector<int>>(m_size);
    for(gsl::index i=0; i<m_size; ++i)
      m_data->at(i) = other.m_values[i];
    m_values = m_data->data();

  }

  gsl::index m_size;
  const double *m_binEdges;
  int *m_values;
  std::unique_ptr<std::vector<int>> m_data{nullptr}; // should also hold data
                                                     // for bin edges in the
                                                     // final implementation.
};

TEST(Histogram, copy_copies_data) {
  Dataset d;
  d.addColumn<double>("name1");
  d.addColumn<int>("name2");
  d.addDimension(Dimension::Tof, 2);
  d.addDimension(Dimension::SpectrumNumber, 10);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  d.extendAlongDimension(ColumnType::Ints, Dimension::Tof);
  d.extendAlongDimension(ColumnType::Ints, Dimension::SpectrumNumber);
  Histogram hist(d, 1); // should only ever live within Dataset, this
                        // constructor would not be public in the final
                        // implementation!
  d.get<int>()[2] = 7;
  ASSERT_EQ(hist.m_values[0], 7);
  auto copy(hist);
  d.get<int>()[2] = 8;
  ASSERT_EQ(hist.m_values[0], 8);
  ASSERT_EQ(copy.m_values[0], 7);
}
