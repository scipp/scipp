/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <memory>
#include <vector>

#include <gsl/gsl_util>

#include "unit.h"

template <class... Ts> class DatasetView;

// Note that this does not and will not support "point data". Will be handled by
// a separate type!
class Histogram {
public:
  Histogram() = default;
  Histogram(const Unit &unit, const gsl::index size, const gsl::index stride,
            const double *edges, double *values, double *errors)
      : m_unit(unit), m_size(size), m_stride(stride), m_edges(edges),
        m_values(values), m_errors(errors) {}

  Histogram(const Histogram &other)
      : m_unit(other.m_unit), m_size(other.m_size), m_stride(other.m_stride) {
    m_data = std::make_unique<std::vector<double>>(3 * m_size + 1);
    for (gsl::index i = 0; i < m_size + 1; ++i)
      m_data->operator[](i) = other.m_edges[i];
    for (gsl::index i = 0; i < m_size; ++i)
      m_data->operator[](m_size + 1 + i) = other.m_values[i];
    for (gsl::index i = 0; i < m_size; ++i)
      m_data->operator[](2 * m_size + 1 + i) = other.m_errors[i];
    m_edges = m_data->data();
    m_values = m_data->data() + m_size + 1;
    m_errors = m_data->data() + 2 * m_size + 1;
  }
  Histogram(Histogram &&other) = default;
  Histogram &operator=(Histogram &&other) = default;

  gsl::index size() const { return m_size; }
  double &value(const gsl::index i) { return m_values[i]; }
  const double &value(const gsl::index i) const { return m_values[i]; }

  template <class... Ts> friend class DatasetView;

private:
  // TODO Unit for Y and E (representing whether we are dealing with
  // count/frequencies standard deviations/variance.
  const Unit m_unit{Unit::Id::Dimensionless};
  const gsl::index m_size{0};
  const gsl::index m_stride{1};
  const double *m_edges{nullptr};
  double *m_values{nullptr};
  double *m_errors{nullptr};
  std::unique_ptr<std::vector<double>> m_data;
};

#if 0
Dataset d1;
Dataset d2;
auto h1 = d1.histogram(i); // Dataset does not contain histograms, this returns by value, but h references d!? Copy constructor, assignment??
auto h2 = d2.histogram(i);
h1 *= 2.0; // scale Y and E in d1
h1 = h2; // compare X in d1 and d2, copy Y and E from d2 to d1
Histogram h;
h = h2; // not in any Dataset

// Same thing, but histograms stored in Dataset, in *addition* to X, Y, E?
Dataset d1;
Dataset d2;
auto &h1 = d1.histogram(i); // reference, h1 references data in d1
auto h2 = d1.histogram(i); // value, copy constructor extracts data, stored internally?
h1 = h2; // sets data in d1, works only if unit ok??



// Histogram convertStdDevToVariance(const Histogram &histogram);


// handle unit in Histogram, not in Values, Errors (to get arround threading issues)? What about BinEdges? will lead to out-of-date units if data columns in dataset are modified!
// just check the unit? handle as other values? reference unit in dataset, propagate for stand-alone histograms
// - how to modify dataset including uit change?
// h1 *= h2; // unit is global to dataset cannot use on histogram level!
// => use d1 *= d2? // no! may not want to edit all columns
// d1.get<Values, Errors> *= d2.get<Values, Errors>; // full column, can handle units
// use column in Dataset for units??
// unit as attribute of Histogram *column*?
// h1 *= h2; // fail, cannot edit unit?
// d1.get<Histograms> *= d2.get<Histograms>; // edits unit
// d1.get<Values> *= d2.get<Values>; // bypasses units??!! :( no! unit stored on BinEdges, Values, and Errors!
//
// Dataset d;
// d.addColumn(BinEdges{});
// d.addColumn(Values{});
// d.addColumn(Errors{});
// d.addHistogram(); // references BinEdges, Values, Errors
// d.addColumn(Units{});
//

Histogram myalg(const Histogram &hist1, const Histogram &hist2) {
  // hist1 *= hist2; // fail because it modifies unit?
  return hist1 * hist2; // creates new histogram with new unit! adds overhead because it prevents in-place operation? -> for more performance, operate with Dataset?
  // How do we put this back into a Dataset? Can it be done in-place? Copy data, ignoring units, set unit once? Don't do it in-place?
  // d1.get<Values>() *= d2.get<Values>(); // ?
  // If histogram referencing data in Dataset cannot be modifed unless unit is unchanged, is it still useful?
}

Dataset d;
d.get<ColumnId::BinEdges>(); // may have different dimension, not useful in combination with Values and Errors below!
d.get<ColumnId::Values>();
d.get<ColumnId::Errors>();

// How should we write rebin()?
Dataset rebin(const Dataset &d, const std::vector<BinEdge> &binEdges); // copies everything?
void rebin(Dataset &d, const Dataset &binEdges) {
  // 1. Add new data (value + error) column to d with same dimensions but different TOF length.
  // 2. Add new binEdges as new column
  // 3.
  for(auto &item : DatasetIterator<OldEdges, OldData, NewEdges, NewData>) { // how can we have several such items (old + new), unless we use strings to identify them?? should have names and Ids?
    rebin(item);
  }
  // 4. Remove old data and old binEdges.

}
// would rather write code at a lower level, not knowing about Dataset! Applying
// to Dataset should happen automatically!


d.apply(rebin)

// Advantages of Dataset:
// - single implementation of extracting/slicing, merging, chopping
// - single implementation for loading/saving and visualization?
// Questions:
// - Arbitrary columns?
//   - How to identify columns in this case? strings feels error prone, and cumbersome, since we also need to use the type: d.get<double>("counts");
//     Allow getting by type, throw if duplicate?!
//   - If not, aren't we too restrictive, e.g., for tables? (could use extra dimension if multiple columns of same type are needed??)
// - should we store Values and Errors as separate columns, or a single column if std::pair<Value, Error>? Is there data without errors?


void apply(Dataset &d, const std::function &f);
// Use signature of f to determine which columns to apply to and which dimensions are core dimensions?
#endif
